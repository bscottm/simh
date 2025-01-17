#if defined(_WIN32) || defined(_WIN64)

#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>

#include "sim_defs.h"
#include "scp.h"
#include "sim_sock.h"
#include "sim_ether.h"
#include "sim_networking/sim_networking.h"
#include "sim_networking/net_support.h"

/* Forward declarations of Windows items that are used by OpenVPN TAPTUN and
 * the "show eth" command. */

/* MS-defined GUID for the network device class. */
const GUID GUID_DEVCLASS_NET = { 0x4d36e972L, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };
const GUID GUID_EMPTY_GUID   = { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };

/* Registry key template for network adapter info. */
const char szAdapterRegKeyPathTemplate[] = "SYSTEM\\CurrentControlSet\\Control\\Network\\%" PRIsLPOLESTR "\\%" PRIsLPOLESTR "\\Connection";

t_stat queryWindowsRegistry(_In_ HKEY hKey, _In_ LPCTSTR szName, _Out_ LPTSTR *pszValue)
{
    if (pszValue == NULL) {
        return SCPE_ARG;
    }

    DWORD dwValueType = REG_NONE, dwSize = 0;
    DWORD dwResult = RegQueryValueEx(hKey, szName, NULL, &dwValueType, NULL, &dwSize);
    if (dwResult != ERROR_SUCCESS) {
        /* No such key... */
        return SCPE_ARG;
    }

    switch (dwValueType) {
        case REG_SZ:
        case REG_EXPAND_SZ:
        {
            /* Read value. */
            LPTSTR szValue = (LPTSTR) malloc(dwSize);
            if (szValue == NULL) {
                return sim_messagef(SCPE_MEM, "%s: malloc(%u) failed", __FUNCTION__, dwSize);
            }

            dwResult = RegQueryValueEx(hKey, szName, NULL, NULL, (LPBYTE)szValue, &dwSize);
            if (dwResult != ERROR_SUCCESS) {
                free(szValue);
                return sim_messagef(SCPE_IOERR, "%s: reading \"%" PRIsLPSTR "\" registry value failed", __FUNCTION__, szName);
            }

            if (dwValueType == REG_EXPAND_SZ) {
                /* Expand the environment strings. */
                DWORD dwSizeExp = dwSize * 2;
                DWORD dwCountExp = dwSizeExp / sizeof(TCHAR) - 1;
                LPTSTR szValueExp = (LPTSTR) malloc(dwSizeExp);
                if (szValueExp == NULL) {
                    free(szValue);
                    return sim_messagef(SCPE_MEM, "%s: malloc(%u) failed", __FUNCTION__, dwSizeExp);
                }

                DWORD dwCountExpResult = ExpandEnvironmentStrings(szValue, szValueExp, dwCountExp );
                if (dwCountExpResult == 0) {
                    free(szValueExp);
                    free(szValue);
                    return sim_messagef(SCPE_IOERR, "%s: expanding \"%" PRIsLPSTR "\" registry value failed",
                                        __FUNCTION__, szName);
                } else if (dwCountExpResult <= dwCountExp) {
                    /* The buffer was big enough. */
                    free(szValue);
                    *pszValue = szValueExp;
                    return SCPE_OK;
                } else {
                    /* Retry with a bigger buffer. */
                    free(szValueExp);
                    /* Note: ANSI version requires one extra char. */
                    dwSizeExp = (dwCountExpResult + 1) * sizeof(char);
                    dwCountExp = dwCountExpResult;
                    szValueExp = (LPSTR) malloc(dwSizeExp);
                    if (szValueExp == NULL) {
                        free(szValue);
                        return sim_messagef(SCPE_MEM, "%s: malloc(%u) failed", __FUNCTION__, dwSizeExp);
                    }

                    dwCountExpResult = ExpandEnvironmentStrings(szValue, szValueExp, dwCountExp);
                    free(szValue);
                    *pszValue = szValueExp;
                    return SCPE_OK;
                }
            } else {
                *pszValue = szValue;
                return SCPE_OK;
            }
        }

        default:
            return sim_messagef(SCPE_ARG, "%s: \"%" PRIsLPSTR "\" registry value is not string (type %u)",
                                __FUNCTION__, dwValueType);
    }
}

/* Look up an adapter's user-defined description in the Windows registry.
 *
 * char *dev_guid: The device's brace-enclosed GUID string, e.g., "{...}"
 * char **description: The returned description, must be free()-ed by the caller.
 */
t_stat windows_eth_dev_description(const char *dev_guid, char **description)
{
    /* Assume something other than SCPE_OK. */
    t_stat retval = SCPE_ARG;

    LPOLESTR szDevClassNetId = NULL;
    StringFromIID((REFIID) &GUID_DEVCLASS_NET, &szDevClassNetId);

    char adapterKey[ADAPTER_REGKEY_PATH_MAX];

    /* Convert the list's GUID to a wide string so we can reuse the template. */
    const int lenDevGUID = (int) ((strlen(dev_guid) + 1) * sizeof(wchar_t));
    LPOLESTR wszDeDevGUID = (LPOLESTR) malloc(lenDevGUID);

    MultiByteToWideChar(CP_ACP, 0, dev_guid, -1, wszDeDevGUID, lenDevGUID);
    int nlen = snprintf(adapterKey, ADAPTER_REGKEY_PATH_MAX, szAdapterRegKeyPathTemplate, szDevClassNetId, wszDeDevGUID);
    free(wszDeDevGUID);

    if (nlen <= ADAPTER_REGKEY_PATH_MAX) {
        /* These registry keys don't seem to exist for all devices, so we simply ignore errors. */
        HKEY hKey = NULL;
        DWORD dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, adapterKey, 0, KEY_READ, &hKey);

        if (dwResult == ERROR_SUCCESS) {
            if (queryWindowsRegistry(hKey, "Name", description) == SCPE_OK) {
                retval = SCPE_OK;
            }
        
            RegCloseKey(hKey);
        }
    } else {
        sim_printf("%s: regkey template overflow\n", __FUNCTION__);
    }

    CoTaskMemFree(szDevClassNetId);
    return retval;
}
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
static MIB_IFTABLE *pIfTable = NULL;

/* The previous pcap_mac_if_win32 implementation use the NPCAP **internal** packet API to
 * acquire the adapter's MAC address and used dynamic library loading from packet.dll to
 * call these internal functions. This approach is inherently unsafe because, as the NPCAP
 * developers repeatedly point out, the packet API is internal and subject to unexpected
 * changes.
 * 
 * Did some research and adapted code to directly query Windows for the MAC address.
 * 
 * It's very likely that packet.dll's API did change because the reimplemented code below
 * does not result in mystery pointer and malloc() problems when SIMH exits.
 */

/* Windows GUID comparison function: Uses wide characters because that's how the
 * GUIDs are returned in a MIB_IFROW structure.
 *
 * Returns:
 * -1: Error in the pcap GUID
 *  0: GUIDs match
 *  1: Error in the Windows interface GUID
 */
static int compare_guid(wchar_t *wszPcapName, wchar_t *wszIfName)
{
    wchar_t *pc, *ic;

    /* Skip to the "{" leader in the pcap GUID: */
    for (pc = wszPcapName; *pc != 0 && *pc != L'{'; ++pc)
        /* NOP*/ ;
    if (*pc == 0)
        return -1;

    ++pc;

    /* Skip to the leader in the interface's GUID. */
    for (ic = wszIfName; *ic != 0 && *ic != L'{'; ++ic)
        /* NOP */ ;
    if (*ic == 0)
        return 1;

    ++ic;

    /* Match the rest of the string */
    for (/* empty */; *pc != 0 && *ic != 0 && *pc != L'}' && *ic != L'}'; ++pc,++ic) {
        if (*pc != *ic)
            return *ic - *pc;
    }

    return ((*pc == 0) * -1) | ((*ic == 0) * 1);
}

/* Look up the AdapterName's MAC address.
 * 
 * Returns:
 *  0: Successful
 * -1: Did not find the adapter's MAC address.
 */
int pcap_mac_if_win32(const char *AdapterName, unsigned char MACAddress[6])
{
    /* Assume failure until proven otherwise. */
    int retval = -1;
    size_t i;

    if (pIfTable != NULL) {
        /* Convert AdapterName into a wide string for comparison to the returned
         * GUIDs from GetIfTable(). */
        wchar_t* wszWideName;
        size_t stISize = strlen(AdapterName) + 1, stOSize;

        if ((wszWideName = (wchar_t *) malloc(stISize * sizeof(wchar_t))) == NULL)
            return retval;

        mbstowcs_s(&stOSize, wszWideName, stISize, AdapterName, stISize);

        for (i = 0; i < pIfTable->dwNumEntries && retval != 0; i++) {
            if (!compare_guid(wszWideName, pIfTable->table[i].wszName) && pIfTable->table[i].dwPhysAddrLen == 6) {
                memcpy(MACAddress, pIfTable->table[i].bPhysAddr, 6);
                retval = 0;
            }
        }

        free(wszWideName);
    } else {
        /* Initial table size. Usually ample. 
         *
         * Note: MIB_IFTABLE is a variably sized array, which is the size
         * of the MIB_IFTABLE structure AND trailing MIB_IFROW structures.
         */
        DWORD dwSize = sizeof(MIB_IFTABLE) + 63 * sizeof(MIB_IFROW);
        DWORD dwRetVal;

        if ((pIfTable = (MIB_IFTABLE *) malloc(dwSize)) == NULL)
            return retval;

        if ((dwRetVal = GetIfTable(pIfTable, &dwSize, FALSE)) == ERROR_INSUFFICIENT_BUFFER) {
            free(pIfTable);
            pIfTable = (MIB_IFTABLE *) malloc(dwSize);
            if (pIfTable == NULL) {
                return retval;
            }

            /* Well, at least it's the exact size needed now... */
            dwRetVal = GetIfTable(pIfTable, &dwSize, FALSE);
        }

        if (dwRetVal == NO_ERROR)
            return pcap_mac_if_win32(AdapterName, MACAddress);

        /* Nope. Got an error. Clean up. */
        free(pIfTable);
        pIfTable = NULL;
    }

    return retval;
}

#endif  /* defined(_WIN32) || defined(__CYGWIN__) */
