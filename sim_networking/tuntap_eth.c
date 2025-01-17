
#include "sim_ether.h"
#include "sim_networking/sim_networking.h"
#include "sim_networking/net_support.h"

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/
/* *nix TUNTAP-based simulated Ethernet implementation:                       */
/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

#if defined(HAVE_TAP_NETWORK)
    static int tuntap_reader(ETH_DEV *eth_dev, int ms_timeout)
    {
#  if  defined(USE_READER_THREAD)
        int retval = netsupport_poll_socket(eth_dev->api_data.tap_sock, ms_timeout);
#  else
        /* Non-blocking/non-AIO needs a value to get past the conditional. */
        int retval = 1;
#  endif

        if (retval > 0) {
            int len;
            u_char buf[ETH_MAX_JUMBO_FRAME];

            len = read(eth_dev->api_data.tap_sock, buf, sizeof(buf));
            if (len > 0) {
                sim_eth_callback(eth_dev, len, len, buf);
            }

            /* retval evaluates to -1 (len < 0), 1 (len > 0) or 0 (len == 0) */
            retval = (len < 0) * -1 + (len > 0) * 1;
        }

        return retval;
    }

    static int tuntap_writer(ETH_DEV *eth_dev, ETH_PACK *packet)
    {
        return (((int) packet->len == write(eth_dev->api_data.tap_sock, (void *) packet->msg, packet->len)) ? 0 : -1);
    }

  /* TUN/TAP API functions*/
  const eth_apifuncs_t tuntap_api_funcs = {
      .reader = tuntap_reader,
      .writer = tuntap_writer,
#  if defined(USE_READER_THREAD)
      .reader_shutdown = default_reader_shutdown,
      .writer_shutdown = default_writer_shutdown
#  endif
  };
#endif /* HAVE_TAP_NETWORK */
