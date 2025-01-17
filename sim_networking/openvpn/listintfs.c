/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/
/* OpenVPN TAP (Win32/Win64) simulated Ethernet implementation:               */
/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

#if defined(WITH_OPENVPN_TAPTUN)

#include <ws2tcpip.h>
#include <windows.h>
#include <SetupAPI.h>

#include "sim_ether.h"
#include "sim_networking/sim_networking.h"
#include "sim_networking/net_support.h"

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * This code was adapted from OpenVPN's tapctl utility's "list" command. It's
 * been stripped down to take advantage of the fact that SIMH uses single byte
 * character sets (i.e., "char" is a byte) vs. multibyte character sets.
 *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

/* The preferred OpenVPN hardware ID. */
#define TAP_WIN_COMPONENT_ID "tap0901"

/* Possible OpenVPN hardware identifiers */
const static char *openvpn_hwids[] = {
  "root\\" TAP_WIN_COMPONENT_ID,
  TAP_WIN_COMPONENT_ID,
  "Wintun",
  "ovpn-dco"
};

/* Default device description if the device's registry entries don't provide one. */
const char *default_openvpn_desc = "OpenVPN TAP device";

/* Predicate function that matches a hardware ID with OpenVPN's possible
 * hardware IDs.
 */
static bool is_openvpn_hwid(LPSTR hwid)
{
    size_t i;

    for (i = 0; i < _countof(openvpn_hwids); ++i) {
        if (!strcmp(hwid, openvpn_hwids[i]))
            return true;
    }

    return false;
}

static t_stat get_net_adapter_guid(_In_ HDEVINFO hDeviceInfoSet, _In_ PSP_DEVINFO_DATA pDeviceInfoData,
                                   _In_ int iNumAttempts, _Out_ LPGUID pguidAdapter)
{
    DWORD dwResult;
    t_stat retval = SCPE_OK;

    if (pguidAdapter == NULL || iNumAttempts < 1) {
        return SCPE_ARG;
    }

    /* Open HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\<class>\<id> registry key. */
    HKEY hKey = SetupDiOpenDevRegKey(hDeviceInfoSet, pDeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
    if (hKey == INVALID_HANDLE_VALUE) {
        return sim_messagef(SCPE_IOERR, "%s: SetupDiOpenDevRegKey failed\n", __FUNCTION__);
    }

    while (iNumAttempts > 0) {
        /* Query the NetCfgInstanceId value. Using queryWindowsRegistry() right on might clutter the output
         * with error messages while the registry is still being populated. */
        LPSTR szCfgGuidString = NULL;
        dwResult = RegQueryValueEx(hKey, "NetCfgInstanceId", NULL, NULL, NULL, NULL);
        if (dwResult != ERROR_SUCCESS) {
            if (dwResult == ERROR_FILE_NOT_FOUND && --iNumAttempts > 0) {
                /* Wait and retry. */
                Sleep(1000);
                continue;
            }

            sim_printf("%s: querying \"NetCfgInstanceId\" registry value failed\n", __FUNCTION__);
            break;
        }

        /* Read the NetCfgInstanceId value now. */
        if (queryWindowsRegistry(hKey, "NetCfgInstanceId", &szCfgGuidString) != SCPE_OK) {
            break;
        }

        const int oleStrLen = (int) ((strlen(szCfgGuidString) + 1) * sizeof(wchar_t));
        LPOLESTR oleCfgGuidString = (LPOLESTR) malloc(oleStrLen);

        /* OLE uses wide characters, so need to temporarily convert... */
        MultiByteToWideChar(CP_ACP, 0, szCfgGuidString, -1, oleCfgGuidString, oleStrLen);
        retval = SUCCEEDED(CLSIDFromString(oleCfgGuidString, (LPCLSID) pguidAdapter)) ? SCPE_OK : SCPE_IOERR;
        free(oleCfgGuidString);
        free(szCfgGuidString);
        break;
    }

    RegCloseKey(hKey);
    return retval;
}

static t_stat get_device_reg_property(_In_ HDEVINFO hDeviceInfoSet, _In_ PSP_DEVINFO_DATA pDeviceInfoData,
                                      _In_ DWORD dwProperty, _Out_opt_ LPDWORD pdwPropertyRegDataType,
                                      _Out_ LPVOID *ppData)
{
    DWORD dwResult;

    if (ppData == NULL) {
        return SCPE_ARG;
    }

    /* Try with stack buffer first. */
    BYTE bBufStack[128];
    DWORD dwRequiredSize = 0;
    if (SetupDiGetDeviceRegistryProperty(hDeviceInfoSet, pDeviceInfoData, dwProperty, pdwPropertyRegDataType,
                                         bBufStack, sizeof(bBufStack), &dwRequiredSize)) {
        /* Copy from stack. */
        *ppData = malloc(dwRequiredSize);
        if (*ppData == NULL) {
            return sim_messagef(SCPE_MEM, "%s: malloc(%u) failed", __FUNCTION__, dwRequiredSize);
        }

        memcpy(*ppData, bBufStack, dwRequiredSize);
        return SCPE_OK;
    } else {
        dwResult = GetLastError();
        if (dwResult == ERROR_INSUFFICIENT_BUFFER) {
            /* Allocate on heap and retry. */
            *ppData = malloc(dwRequiredSize);
            if (*ppData == NULL) {
                return sim_messagef(SCPE_MEM, "%s: malloc(%u) failed", __FUNCTION__, dwRequiredSize);
            }

            if (SetupDiGetDeviceRegistryProperty(hDeviceInfoSet, pDeviceInfoData, dwProperty, pdwPropertyRegDataType,
                                                 *ppData, dwRequiredSize, &dwRequiredSize)) {
                return SCPE_OK;
            } else {
                sim_messagef(SCPE_IOERR, "%s: SetupDiGetDeviceRegistryProperty(%u) failed", __FUNCTION__, dwProperty);
            }
        } else {
            return sim_messagef(SCPE_IOERR, "%s: SetupDiGetDeviceRegistryProperty(%u) failed", __FUNCTION__, dwProperty);
        }
    }

    /* Not reached, return a bogus status anyway... */
    return SCPE_NOFNC;
}

/* Now, for the main event! */
int openvpn_tap_devices(ETH_LIST *ethDevices, const int maxList)
{
    DWORD dwResult;
    int curElt = 0;

    /* Get the console's window handle. N.B. Microsoft says that GetConsoleWindow()
     * will be supported indefinitely, but its use should be avoided. Examples
     * abound on how to get the console's window handle by other means. */
    const int lenConsoleTitle = 1024;
    LPSTR consoleTitle = (char *) malloc(lenConsoleTitle);

    if (consoleTitle == NULL)
        return 0;

    GetConsoleTitle(consoleTitle, lenConsoleTitle);

    HWND hwndParent = FindWindow(NULL, consoleTitle);

    free(consoleTitle);

    if (hwndParent == INVALID_HANDLE_VALUE)
        return 0;

    /* Create a list of network devices. */
    HDEVINFO hDevInfoList = SetupDiGetClassDevsEx(&GUID_DEVCLASS_NET, NULL, hwndParent, DIGCF_PRESENT, NULL, NULL, NULL);
    if (hDevInfoList == INVALID_HANDLE_VALUE) {
        sim_messagef(SCPE_IOERR, "%s: SetupDiGetClassDevsEx failed\n", __FUNCTION__);
        return 0;
    }

    /* Retrieve information associated with a device information set. */
    SP_DEVINFO_LIST_DETAIL_DATA devinfo_list_detail_data = { .cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA) };
    if (!SetupDiGetDeviceInfoListDetail(hDevInfoList, &devinfo_list_detail_data)) {
        sim_printf("%s: SetupDiGetDeviceInfoListDetail failed\n", __FUNCTION__);
        goto cleanup_hDevInfoList;
    }

    /* Get the device class GUID as string. */
    LPOLESTR szDevClassNetId = NULL;
    StringFromIID((REFIID) &GUID_DEVCLASS_NET, &szDevClassNetId);

    /* Iterate. */
    DWORD dwIndex;
    for (dwIndex = 0; curElt < maxList; dwIndex++) {
        /* Get the device from the list. */
        SP_DEVINFO_DATA devinfo_data = { .cbSize = sizeof(SP_DEVINFO_DATA) };
        if (!SetupDiEnumDeviceInfo(hDevInfoList, dwIndex, &devinfo_data)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                break;
            } else {
                /* Something is wrong with this device. Skip it. */
                sim_printf("%s: SetupDiEnumDeviceInfo(%u) failed\n", __FUNCTION__, dwIndex);
                continue;
            }
        }

        /* Get device hardware ID(s). */
        DWORD dwDataType = REG_NONE;
        LPSTR szzDeviceHardwareIDs = NULL;
        if (get_device_reg_property(hDevInfoList, &devinfo_data, SPDRP_HARDWAREID, &dwDataType,
                                    (LPVOID)&szzDeviceHardwareIDs) != SCPE_OK) {
            /* Something is wrong with this device. Skip it. */
            continue;
        }

        /* Check that hardware ID is REG_SZ/REG_MULTI_SZ, and it matches the OpenVPN TAP
         * product ID. */
        if (dwDataType == REG_SZ && !is_openvpn_hwid(szzDeviceHardwareIDs)) {
            goto cleanup_szzDeviceHardwareIDs;
        } else if (dwDataType == REG_MULTI_SZ) {
            LPSTR s;

            for (s = szzDeviceHardwareIDs; ; s += strlen(s) + 1) {
                if (*s == '\0') {
                    /* End of strings... */
                    goto cleanup_szzDeviceHardwareIDs;
                } else if (is_openvpn_hwid(s)) {
                    break;
                }
            }
        } else {
            /* Unexpected hardware ID format. Skip device. */
            continue;
        }

        /* Get the hardware description. */
        LPSTR szHardwareDesc = NULL;

        dwDataType = REG_NONE;
        if (get_device_reg_property(hDevInfoList, &devinfo_data, SPDRP_DEVICEDESC, &dwDataType,
                                    (LPVOID) &szHardwareDesc) != SCPE_OK) {
            /* Didn't get a hardware device description, but we know it's OpenVPN's
             * device. */
            szHardwareDesc = (LPSTR) malloc(_countof(default_openvpn_desc) + 1);
            strcpy(szHardwareDesc, default_openvpn_desc);
        }

        /* Get adapter GUID. */
        GUID guidAdapter;
        if (get_net_adapter_guid(hDevInfoList, &devinfo_data, 1, &guidAdapter) != SCPE_OK) {
            /* Something is wrong with this device. Skip it. */
            continue;
        }

        /* Get the adapter GUID as string. */
        LPOLESTR szAdapterId = NULL;
        StringFromIID((REFIID) &guidAdapter, &szAdapterId);

        /* Render registry key path. */
        char szRegKey[ADAPTER_REGKEY_PATH_MAX];
        int lenRegKey;

        lenRegKey = snprintf(szRegKey, _countof(szRegKey), szAdapterRegKeyPathTemplate, szDevClassNetId, szAdapterId);
        if (lenRegKey >= ADAPTER_REGKEY_PATH_MAX) {
            sim_printf("%s: Register key overflow (%d written, %d max)\n", lenRegKey, ADAPTER_REGKEY_PATH_MAX);
            goto cleanup_szAdapterId;
        }

        /* Open network adapter registry key. */
        HKEY hKey = NULL;
        dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0, KEY_READ, &hKey);
        if (dwResult != ERROR_SUCCESS) {
            sim_printf("%s: RegOpenKeyEx(HKLM, \"%" PRIsLPSTR "\") failed\n", __FUNCTION__, szRegKey);
            goto cleanup_szAdapterId;
        }

        /* Read adapter name. */
        LPSTR szName = NULL;
        dwResult = queryWindowsRegistry(hKey, "Name", &szName);
        if (dwResult != ERROR_SUCCESS) {
            sim_printf("%s: Cannot determine %" PRIsLPOLESTR " adapter name\n", __FUNCTION__, szAdapterId);
            goto cleanup_hKey;
        }

        /* Append to the list. */
        size_t name_size = (strlen(szName) + 1) * sizeof(char);

        memcpy(ethDevices[curElt].name, szName, name_size);
        strlcpy(ethDevices[curElt].desc, szHardwareDesc, ETH_DEV_DESC_MAX);
        /* Make sure both strings are terminated properly. */
        ethDevices[curElt].name[ETH_DEV_NAME_MAX-1] = ethDevices[curElt].desc[ETH_DEV_DESC_MAX-1] = '\0';
        ethDevices[curElt].eth_api = ETH_API_TAP;
        ethDevices[curElt].is_openvpn = 1;
        ethDevices[curElt].adapter_guid = guidAdapter;
        ++curElt;

        free(szName);

cleanup_hKey:
        RegCloseKey(hKey);
cleanup_szAdapterId:
        free(szHardwareDesc);
        CoTaskMemFree(szAdapterId);
cleanup_szzDeviceHardwareIDs:
        free(szzDeviceHardwareIDs);
    }

    CoTaskMemFree(szDevClassNetId);

cleanup_hDevInfoList:
    SetupDiDestroyDeviceInfoList(hDevInfoList);
    return curElt;
}
#endif
