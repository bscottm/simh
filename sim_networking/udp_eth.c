#include "sim_defs.h"
#include "sim_sock.h"
#include "sim_ether.h"
#include "sim_networking/sim_networking.h"
#include "sim_networking/net_support.h"

#if defined(USE_READER_THREAD)
static int udp_reader(ETH_DEV *eth_dev, int ms_timeout)
{
    int retval = netsupport_poll_socket(eth_dev->api_data.udp_sock, ms_timeout);

    if (retval > 0) {
        int len;
        u_char buf[ETH_MAX_JUMBO_FRAME];

        len = (int) sim_read_sock (eth_dev->api_data.udp_sock, (char *) buf, (int32) sizeof(buf));
        if (len > 0) {
            sim_eth_callback(eth_dev, len, len, buf);
        }

        /* retval evaluates to -1 (len < 0), 1 (len > 0) or 0 (len == 0) */
        retval = (len < 0) * -1 + (len > 0) * 1;
    }

    return retval;
}
#endif

static int udp_writer(ETH_DEV *eth_dev, ETH_PACK *packet)
{
  return (((int) packet->len == sim_write_sock(eth_dev->api_data.udp_sock, (char *) packet->msg, (int) packet->len)) ? 0 : -1);
}

const eth_apifuncs_t udp_api_funcs = {
#if defined(USE_READER_THREAD)
    .reader = udp_reader,
#endif
    .writer = udp_writer,
#if defined(USE_READER_THREAD)
    .reader_shutdown = default_reader_shutdown,
    .writer_shutdown = default_writer_shutdown
#  endif
};
