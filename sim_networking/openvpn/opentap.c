#if defined(WITH_OPENVPN_TAPTUN)

#define WINDOWS_LEAN_AND_MEAN 1
#include <ws2tcpip.h>
#include <windows.h>
#include <tap-windows.h>

#include "sim_defs.h"
#include "sim_sock.h"
#include "sim_ether.h"
#include "sim_networking/sim_networking.h"
#include "sim_networking/net_support.h"


t_stat openvpn_open(const char *cmdstr)
{
    const char *devstr = cmdstr + 4, *tuntap_devname;
    ETH_LIST dev_list[ETH_MAX_DEVICE];
    int ndevs;
    GUID tap_guid;
    char tuntap_path[256];

    ndevs = eth_devices(ETH_MAX_DEVICE, dev_list, FALSE);

    while (isspace(*devstr))
        ++devstr;

    /* Get the TAP device's GUID */
    if (*devstr == '"') {
        tuntap_devname = ++devstr;
        while (*devstr && *devstr != '"')
            ++devstr;
        if (!*devstr || *devstr != '"') {
            return sim_messagef(SCPE_OPENERR, "Unterminated tap device name string.\n");
        }
    } else {
        tuntap_devname = devstr;
        while (*devstr && !isspace(*devstr))
            ++devstr;
    }

    size_t i;

    for (i = 0; i < ndevs && strncmp(dev_list[i].name, tuntap_devname, devstr - tuntap_devname); ++i)
        /* NOP */ ;
    if (i < ndevs && dev_list[i].is_openvpn && memcmp(&dev_list[i].adapter_guid, &GUID_EMPTY_GUID, sizeof(GUID))) {
        tap_guid = dev_list[i].adapter_guid;
    } else {
        return sim_messagef(SCPE_OPENERR, "No such OpenVPN TAP device.\n");
    }

    /* Open the OpenVPN TAP device: */
    OLECHAR szTapGUID[40];
    
    StringFromGUID2(&tap_guid, szTapGUID, _countof(szTapGUID));
    sprintf(tuntap_path, "%s%ls%s", USERMODEDEVICEDIR, szTapGUID, TAP_WIN_SUFFIX);

    HANDLE tapdev = CreateFile(
            tuntap_path,
            MAXIMUM_ALLOWED,
            0,
            0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED,
            0
            );

    if (tapdev != INVALID_HANDLE_VALUE) {
        CloseHandle(tapdev);
    } else {
        return sim_messagef(SCPE_OPENERR, "Unable to open OpenVPN TAP device.\n");
    }

    return SCPE_OK;
}
#endif