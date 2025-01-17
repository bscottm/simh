#include "sim_defs.h"
#include "sim_sock.h"
#include "sim_ether.h"
#include "sim_networking/sim_networking.h"
#include "sim_networking/net_support.h"

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/
/* VDE (Virtual Distributed Ethernet) simulated Ethernet implementation:      */
/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

#if defined(HAVE_VDE_NETWORK)
    static int vde_reader(ETH_DEV *eth_dev, int ms_timeout)
    {
#if  defined(USE_READER_THREAD)
        int retval = netsupport_poll_socket(eth_dev->api_data.vde.vde_sock, ms_timeout);
#else
        int retval = 1;
#endif

        if (retval > 0) {
            int len;
            u_char buf[ETH_MAX_JUMBO_FRAME];

            len = vde_recv(eth_dev->api_data.vde.vde_conn, buf, sizeof(buf), 0);
            if (len > 0) {
                sim_eth_callback(eth_dev, len, len, buf);
            }

            /* retval evaluates to -1 (len < 0), 1 (len > 0) or 0 (len == 0) */
            retval = (len < 0) * -1 + (len > 0) * 1;
        }

        return retval;
    }

  static int vde_writer(ETH_DEV *eth_dev, ETH_PACK *packet)
  {
      int status = vde_send(eth_dev->api_data.vde.vde_conn, (void *) packet->msg, packet->len, 0);

      if ((status == (int)packet->len) || (status == 0))
          status = 0;
      else
          if ((status == -1) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
              status = 0;
          else
              status = 1;

      return status;
  }

  /* VDE API functions */
  const eth_apifuncs_t vde_api_funcs = {
      .reader = vde_reader,
      .writer = vde_writer,
#  if defined(USE_READER_THREAD)
      .reader_shutdown = default_reader_shutdown,
      .writer_shutdown = default_writer_shutdown
#  endif
  };
#endif /* HAVE_VDE_NETWORK */
