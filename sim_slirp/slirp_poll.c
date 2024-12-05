/* sim_slirp.c:
  ------------------------------------------------------------------------------
   Copyright (c) 2024, B. Scott Michel

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of the author shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from the author.

  ------------------------------------------------------------------------------

   This module encapsulates the poll()/select() interface to libslirp.

*/

#include <glib.h>
#include <errno.h>                  /* Paranoia for Win32/64 */
#include <limits.h>

#include "libslirp.h"
#include "sim_defs.h"
#include "scp.h"
#include "sim_sock.h"
#include "sim_slirp_network.h"
#include "sim_printf_fmts.h"

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif

#if defined(_WIN64)
#  if defined(PRIu64)
#    define SIM_PRIsocket PRIu64
#  else
#    define SIM_PRIsocket "llu"
#  endif
#elif defined(_WIN32)
#  if defined(PRIu32)
#    define SIM_PRIsocket PRIu32
#  else
#    define SIM_PRIsocket "lu"
#  endif
#else
#  define SIM_PRIsocket "u"
#endif

/* Forward decl's: */
static int add_poll_callback(slirp_os_socket fd, int events, void *opaque);
static int get_events_callback(int idx, void *opaque);
static void simh_timer_check(Slirp *slirp);

/* Protocol functions: */
static void initialize_poll(SimSlirpNetwork *slirp, uint32 tmo, struct timeval *tv);
static int do_poll(SimSlirpNetwork *slirp, int ms_timeout, struct timeval *timeout);
static void report_error(SimSlirpNetwork *slirp);

static inline uint32_t slirp_dbg_mask(const SimSlirpNetwork *slirp, size_t flag)
{
  return (slirp != NULL && slirp->dptr != NULL) ?  slirp->dptr->debflags[slirp->flag_offset + flag].mask : 0;
}

#if SIM_USE_SELECT
/* Socket identifier pretty-printer: */
static const char *print_socket(slirp_os_socket s)
{
    static char retbuf[64];

#if defined(_WIN64)
    sprintf(retbuf, "%llu", s);
#elif defined(_WIN32)
    sprintf(retbuf, "%u", s);
#else
    sprintf(retbuf, "%d", s);
#endif

    return retbuf;
}
#endif

/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 * The libslirp poll()/select() interface:
 *
 * sim_slirp_select() is simply a driver function that invokes three "protocol" functions to initialize, poll and
 * report an error. The protocol functions isolate functionality without making sim_slirp_select() overly complex.
 *~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

int sim_slirp_select(SimSlirpNetwork *slirp, int ms_timeout)
{
    int    retval = 0;
    uint32 slirp_timeout = ms_timeout;
    struct timeval tv;

    if (slirp == NULL)
        /* Will cause the reader thread to exit! */
        return -1;

    /* Note: It's a generally good practice to acquire a mutex and hold it until an operation
     * completes. In this case, though, poll()/select() will block and prevent outbound writes
     * via sim_slirp_send(), which itself blocks waiting for the mutex.
     *
     * Hold the mutex only when calling libslirp functions, reacquire when needed.
     */
    pthread_mutex_lock(&slirp->libslirp_access);

    /* Check on expiring libslirp timers. */
    simh_timer_check(slirp->slirp_cxn);

    /* Ask libslirp to poll for I/O events. */
    initialize_poll(slirp, slirp_timeout, &tv);
    slirp_pollfds_fill_socket(slirp->slirp_cxn, &slirp_timeout, add_poll_callback, slirp);

    pthread_mutex_unlock(&slirp->libslirp_access);

    retval = do_poll(slirp, ms_timeout, &tv);

    if (retval > 0) {
        /* The libslirp idiom invokes slirp_pollfds_poll within the same function as slirp_pollfds_fill(). */
        pthread_mutex_lock(&slirp->libslirp_access);
        slirp_pollfds_poll(slirp->slirp_cxn, 0, get_events_callback, slirp);
        pthread_mutex_unlock(&slirp->libslirp_access);
    } else if (retval < 0) {
        report_error(slirp);
    }

    return retval;
}

/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 * "Protocol" functions. These functions abide by the one function definition rule.
 *~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

#if (defined(_WIN32) || defined(_WIN64)) && SIM_USE_POLL
static inline int poll(WSAPOLLFD *fds, size_t n_fds, int timeout)
{
  return WSAPoll(fds, (ULONG) n_fds, timeout);
}
#endif

static void initialize_poll(SimSlirpNetwork *slirp, uint32 tmo, struct timeval *tv)
{
#if SIM_USE_SELECT
    tv->tv_sec = tmo / 1000;
    tv->tv_usec = (tmo % 1000) * 1000;

    FD_ZERO(&slirp->readfds);
    FD_ZERO(&slirp->writefds);
    FD_ZERO(&slirp->exceptfds);

    slirp->max_fd = SIM_INVALID_MAX_FD;
#elif SIM_USE_POLL
    /* Reinitialize and reset */
    memset(slirp->fds, 0, slirp->fd_idx * sizeof(sim_pollfd_t));
    slirp->fd_idx = 0;
#endif
}

static int do_poll(SimSlirpNetwork *slirp, int ms_timeout, struct timeval *timeout)
{
    int retval = 0;

#if SIM_USE_SELECT
#  if defined(_WIN32)
    /* Windows: The first argument to select(), nfds, is ignored by winsock2. You could pass a
     * random value and winsock2 wouldn't care. 0xdeadbeef seems appropriate.
     *
     * The only way to know whether there is work to be done is to examine the fd_count member in
     * the fd_set structure.
     */
    if (slirp->readfds.fd_count + slirp->writefds.fd_count + slirp->exceptfds.fd_count > 0) {
        retval = select(/* ignored */ 0xdeadbeef, &slirp->readfds, &slirp->writefds, &slirp->exceptfds, timeout);
        if (retval > 0)
            sim_debug(slirp_dbg_mask(slirp, DBG_POLL), slirp->dptr,
                      "do_poll(): select() returned %d (read: %u, write: %u, except: %u)\n",
                      retval, slirp->readfds.fd_count, slirp->writefds.fd_count, slirp->exceptfds.fd_count);
    }
#  else
    if (slirp->max_fd != SIM_INVALID_MAX_FD) {
        retval = select(slirp->max_fd + 1, &slirp->readfds, &slirp->writefds, &slirp->exceptfds, &imeout);
        if (retval > 0)
            sim_debug(slirp_dbg_mask(slirp, DBG_POLL), slirp->dptr, "do_poll(): select() returned %d\n", retval);
    }
#  endif
#elif SIM_USE_POLL
    if (slirp->fd_idx > 0) {
        sim_debug(slirp_dbg_mask(slirp, DBG_POLL), slirp->dptr,
                  "poll()-ing for %" SIZE_T_FMT "u sockets\n", slirp->fd_idx);

        retval = poll(slirp->fds, slirp->fd_idx, ms_timeout);
        if (retval > 0)
            sim_debug(slirp_dbg_mask(slirp, DBG_POLL), slirp->dptr, "do_poll(): poll() returned %d\n", retval);
    }
#endif

    return retval;
}

static void report_error(SimSlirpNetwork *slirp)
{
#if defined(_WIN32)
    char *wsaMsgBuffer = NULL;
    int wsaError = WSAGetLastError();
        
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    WSAGetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* The user default language */
                    (LPTSTR) &wsaMsgBuffer,
                    0,
                    NULL);

    if (sim_deb != stdout && sim_deb != stderr) {
        fprintf(stderr, "sim_slirp_select() winsock error: %d (%s)", wsaError, wsaMsgBuffer);
        fflush(stderr);
    }
    sim_messagef(SCPE_IERR, "sim_slirp_select(): WSAGetLastError() %d (%s)\n", wsaError, wsaMsgBuffer);
    LocalFree(wsaMsgBuffer);
#else
    sim_messagef(SCPE_IERR, "sim_slirp_select(): do_poll() -- errno = %d, %s\n", errno, g_strerror(errno));
#endif
}

/* Add new socket file descriptors to the Slirp I/O event tracking state. */
void register_poll_socket(slirp_os_socket fd, void *opaque)
{
#if SIM_USE_SELECT
    SimSlirpNetwork *slirp = (SimSlirpNetwork *) opaque;
    size_t i;

    for (i = 0; i < slirp->lut_alloc; ++i) {
        if (slirp->lut[i] == INVALID_SOCKET) {
            slirp->lut[i] = fd;
            break;
        }
    }

    if (i == slirp->lut_alloc) {
        SOCKET *new_lut;
        size_t j = slirp->lut_alloc;

        /* Linear growth. */
        slirp->lut_alloc += FDS_ALLOC_INCR;
        new_lut = (SOCKET *) realloc(slirp->lut, slirp->lut_alloc * sizeof(SOCKET));
        ASSURE(new_lut != NULL);
        for (/* empty */; j < slirp->lut_alloc; ++j)
            new_lut[j] = INVALID_SOCKET;

        slirp->lut = new_lut;
        slirp->lut[i] = fd;
    }

    sim_debug(slirp_dbg_mask(slirp, DBG_SOCKET), slirp->dptr, "register_poll_socket(%s) index %" SIZE_T_FMT "d\n",
              print_socket(fd), i);
#elif SIM_USE_POLL
    /* Not necessary for poll(). */
    (void) opaque;
#endif
}

/* Reap a disused socket. */
void unregister_poll_socket(slirp_os_socket fd, void *opaque)
{
    GLIB_UNUSED_PARAM(opaque);

#if SIM_USE_SELECT
    SimSlirpNetwork *slirp = (SimSlirpNetwork *) opaque;
    size_t i;

    for (i = 0; i < slirp->lut_alloc; ++i) {
        if (slirp->lut[i] == fd) {
            slirp->lut[i] = INVALID_SOCKET;
            sim_debug(slirp_dbg_mask(slirp, DBG_SOCKET), slirp->dptr,
                      "unregister_poll_socket(%s) index %" SIZE_T_FMT "d\n", print_socket(fd), i);
            break;
        }
    }
#elif SIM_USE_POLL
    /* Not necessary for poll(). */
    (void) opaque;
#endif
}

/* Debugging output for add/get event callbacks. */
static void poll_debugging(uint32_t dbits, DEVICE *dev, const char *prefix, int events)
{
    static struct transtab {
        int slirp_event;
        const char *slirp_event_str;
    } translations[] = {
        { SLIRP_POLL_IN,  " SLIRP_POLL_IN" },
        { SLIRP_POLL_OUT, " SLIRP_POLL_OUT" },
        { SLIRP_POLL_PRI, " SLIRP_POLL_PRI" },
        { SLIRP_POLL_ERR, " SLIRP_POLL_ERR" },
        { SLIRP_POLL_HUP, " SLIRP_POLL_HUP" }
    };
    static const size_t n_translations = sizeof(translations) / sizeof(translations[0]);
    char *msg = calloc(96 + strlen(prefix) + 1, sizeof(char));

    strcpy(msg, prefix);
    if (events != 0) {
      size_t i;

      strcat(msg, ": ");
      for (i = 0; i < n_translations; ++i) {
          if (translations[i].slirp_event & events)
              strcat(msg, translations[i].slirp_event_str);
      }
    } else {
      strcat(msg, ": [no events]");
    }
    strcat(msg, "\n");

    /* Don't need to escape "%"-s in msg, since there aren't any. */
    sim_debug(dbits, dev, "%s", msg);
    if (sim_deb != NULL)
        fflush(sim_deb);
    free(msg);
}

/* select()/poll() callback: For the sockets that Slirp (slirp_cxn) needs to examine for
 * events, this callback adds them to the select file descriptor set or the poll array. */
static int add_poll_callback(slirp_os_socket fd, int events, void *opaque)
{
    SimSlirpNetwork *slirp = (SimSlirpNetwork *) opaque;
    int retval = -1;
    char prefix[128];

#if SIM_USE_SELECT
    size_t i;
    const int event_mask = (SLIRP_POLL_IN | SLIRP_POLL_OUT | SLIRP_POLL_PRI);

    for (i = 0; i < slirp->lut_alloc; ++i) {
        if (slirp->lut[i] == fd) {
            if (events & SLIRP_POLL_IN) {
                FD_SET(fd, &slirp->readfds);
            }
            if (events & SLIRP_POLL_OUT) {
                FD_SET(fd, &slirp->writefds);
            }
            if (events & SLIRP_POLL_PRI) {
                FD_SET(fd, &slirp->exceptfds);
            }

            retval = (int) i;

            if (slirp->max_fd == SIM_INVALID_MAX_FD || fd > slirp->max_fd)
                slirp->max_fd = fd;

            break;
        }

        sprintf(prefix, "add_poll_callback(%s)/select (0x%04x)", print_socket(fd), events & event_mask);
        poll_debugging(slirp_dbg_mask(slirp, DBG_POLL), slirp->dptr, prefix, events & event_mask);
    }
#elif SIM_USE_POLL
    if (slirp->fd_idx == slirp->n_fds) {
        size_t j = slirp->n_fds;
        sim_pollfd_t *new_fds;

        slirp->n_fds += FDS_ALLOC_INCR;
        new_fds = (sim_pollfd_t *) realloc(slirp->fds, slirp->n_fds * sizeof(sim_pollfd_t));
        ASSURE(new_fds != NULL);
        memset(new_fds + j, 0, (slirp->n_fds - j) * sizeof(sim_pollfd_t));
        slirp->fds = new_fds;
    }

    short poll_events = 0;

    if (events & SLIRP_POLL_IN) {
        poll_events |= POLLIN;
    }
    if (events & SLIRP_POLL_OUT) {
        poll_events |= POLLOUT;
    }
#if !defined(_WIN32) && !defined(_WIN64)
    /* Not supported on Windows. Unless you like EINVAL. :-) */
    if (events & SLIRP_POLL_PRI) {
        poll_events |= POLLPRI;
    }
    if (events & SLIRP_POLL_ERR) {
        poll_events |= POLLERR;
    }
    if (events & SLIRP_POLL_HUP) {
        poll_events |= POLLHUP;
    }
#endif

    sprintf(prefix, "add_poll_callback(%" SIM_PRIsocket ")/poll (0x%04x)", fd, events);
    poll_debugging(slirp_dbg_mask(slirp, DBG_POLL), slirp->dptr, prefix, events);

    slirp->fds[slirp->fd_idx].fd = fd;
    slirp->fds[slirp->fd_idx].events = poll_events;
    slirp->fds[slirp->fd_idx].revents = 0;

    retval = (int) slirp->fd_idx++;
#endif

    return retval;
}

static int get_events_callback(int idx, void *opaque)
{
    const SimSlirpNetwork *slirp = (SimSlirpNetwork *) opaque;
    int event = 0;
    char prefix[128];

#if SIM_USE_SELECT
    const int event_mask = (SLIRP_POLL_IN | SLIRP_POLL_OUT | SLIRP_POLL_PRI);

    if (idx >= 0 && (size_t) idx < slirp->lut_alloc) {
        slirp_os_socket fd = slirp->lut[idx];

        if (FD_ISSET(fd, &slirp->readfds)) {
            event |= SLIRP_POLL_IN;
        }
        if (FD_ISSET(fd, &slirp->writefds)) {
            event |= SLIRP_POLL_OUT;
        }
        if (FD_ISSET(fd, &slirp->exceptfds)) {
            event |= SLIRP_POLL_PRI;
        }

        sprintf(prefix, "get_events_callback(%s)/select (0x%04x)", print_socket(fd), event & event_mask);
        poll_debugging(slirp_dbg_mask(slirp, DBG_POLL), slirp->dptr, prefix, event & event_mask);
    }
#elif SIM_USE_POLL
    if (idx < slirp->fd_idx) {
        short poll_events = slirp->fds[idx].revents;

        if (poll_events & POLLIN) {
            event |= SLIRP_POLL_IN;
        }
        if (poll_events & POLLOUT) {
            event |= SLIRP_POLL_OUT;
        }
        if (poll_events & POLLPRI) {
            event |= SLIRP_POLL_PRI;
        }
        if (poll_events & POLLERR) {
            event |= SLIRP_POLL_ERR;
        }
        if (poll_events & POLLHUP) {
            event |= SLIRP_POLL_HUP;
        }

        sprintf(prefix, "get_events_callback(%" SIM_PRIsocket ")/poll (0x%04x)", slirp->fds[idx].fd, event);
        poll_debugging(slirp_dbg_mask(slirp, DBG_POLL), slirp->dptr, prefix, event);
    }
#endif

    return event;
}


/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 * libslirp timer management: This exclusively applies to IPv6.
 *~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

static struct slirp_timer_s {
    SlirpTimerId id;                /*< libslirp's timer identifier */
    void *cb_opaque;                /*< Opaque thing to pass to the callback */
    int64_t expire_time_ns;         /*< Expiration time, nanoseconds */
    struct slirp_timer_s *next;
} *slirp_timer_list = NULL;

void *simh_timer_new_opaque(SlirpTimerId id, void *cb_opaque, void *opaque)
{
    struct slirp_timer_s *retval = (struct slirp_timer_s *) calloc(1, sizeof(struct slirp_timer_s));

    GLIB_UNUSED_PARAM(opaque);

    retval->id = id;
    retval->cb_opaque = cb_opaque;

    retval->next = slirp_timer_list;
    slirp_timer_list = retval;

    return retval;
}

void simh_timer_free(void *the_timer, void *opaque)
{
    struct slirp_timer_s  *timer = the_timer;
    struct slirp_timer_s **t;

    GLIB_UNUSED_PARAM(opaque);

    for (t =  &slirp_timer_list; *t != NULL && *t != timer; t = &((*t) ->next))
                /* empty */;

    if (*t != NULL)
        *t = timer->next;

    free(timer);
}

void simh_timer_mod(void *the_timer, int64_t expire_time, void *opaque)
{
    struct slirp_timer_s  *timer = the_timer;
    struct slirp_timer_s **t;

    GLIB_UNUSED_PARAM(opaque);

    /* Upconvert to nanoseconds, unexpire. */
    timer->expire_time_ns = expire_time * 1000ll * 1000ll;

    /* Remove from the list. */
    for (t =  &slirp_timer_list; *t != NULL && *t != timer; t = &((*t)->next))
                /* empty */;

    if (*t != NULL)
        *t = timer->next;

    /* Reinsert in sorted order. */
    for (t =  &slirp_timer_list; *t != NULL && expire_time < (*t)->expire_time_ns; t = &((*t)->next))
                /* empty */;

    timer->next = *t;
    *t = timer;
}

void simh_timer_check(Slirp *slirp)
{
    int64_t time_now = sim_clock_get_ns(NULL);
    struct slirp_timer_s *t;

    GLIB_UNUSED_PARAM(slirp);

    for (t= slirp_timer_list; t != NULL && t->expire_time_ns <= time_now; /* empty */) {
        /* libslirp's timer function that invoked us might call simh_timer_mod(), which
         * potentially moves t->next. Keep track of the next element separately so that
         * we keep synchronized with the list's order. */
        struct slirp_timer_s *t_next = t->next;

        slirp_handle_timer(slirp, t->id, t->cb_opaque);
        t = t_next;
    }
}
