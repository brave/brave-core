/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/utils_win.h"

#include <ras.h>
#include <raserror.h>
#include <stdio.h>

#include "base/logging.h"
#include "base/strings/stringprintf.h"

#define DEFAULT_PHONE_BOOK NULL

namespace brave_vpn {

namespace {

// https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
void PrintSystemError(DWORD error) {
  constexpr DWORD kBufSize = 512;
  TCHAR lpsz_error_string[kBufSize];

  DWORD buf_len =
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    lpsz_error_string, kBufSize, NULL);
  if (buf_len) {
    LOG(ERROR) << lpsz_error_string;
  }
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rassetcredentialsa
DWORD SetCredentials(LPCTSTR entry_name, LPCTSTR username, LPCTSTR password) {
  RASCREDENTIALS credentials;

  ZeroMemory(&credentials, sizeof(RASCREDENTIALS));
  credentials.dwSize = sizeof(RASCREDENTIALS);
  credentials.dwMask = RASCM_UserName | RASCM_Password;

  wcscpy_s(credentials.szUserName, UNLEN + 1, username);
  wcscpy_s(credentials.szPassword, PWLEN + 1, password);

  DWORD dw_ret =
      RasSetCredentials(DEFAULT_PHONE_BOOK, entry_name, &credentials, FALSE);
  if (dw_ret != ERROR_SUCCESS) {
    internal::PrintRasError(dw_ret);
    return dw_ret;
  }

  return ERROR_SUCCESS;
}

}  // namespace

namespace internal {

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgeterrorstringa
void PrintRasError(DWORD error) {
  constexpr DWORD kBufSize = 512;
  TCHAR lpsz_error_string[kBufSize];

  if (error > RASBASE && error < RASBASEEND) {
    if (RasGetErrorString(error, lpsz_error_string, kBufSize) ==
        ERROR_SUCCESS) {
      LOG(ERROR) << lpsz_error_string;
      return;
    }
  }

  PrintSystemError(error);
}

std::wstring GetPhonebookPath() {
  TCHAR* app_data_path = NULL;
  std::wstring phone_book_path;

  // https://docs.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-expandenvironmentstringsa
  // Caculate required buf size first.
  const DWORD dw_buf_size =
      ExpandEnvironmentStrings(TEXT("%APPDATA%"), app_data_path, 0);
  if (dw_buf_size == 0) {
    LOG(ERROR) << "failed to use ExpandEnvironmentStrings";
    PrintRasError(GetLastError());
    return phone_book_path;
  }

  // Allocate required buf.
  app_data_path = reinterpret_cast<TCHAR*>(HeapAlloc(
      GetProcessHeap(), HEAP_ZERO_MEMORY, dw_buf_size * sizeof(TCHAR)));
  if (app_data_path == NULL) {
    LOG(ERROR) << "HeapAlloc failed!";
    return app_data_path;
  }

  ExpandEnvironmentStrings(TEXT("%APPDATA%"), app_data_path, dw_buf_size);

  phone_book_path = base::StringPrintf(
      L"%ls\\Microsoft\\Network\\Connections\\Pbk\\rasphone.pbk",
      app_data_path);

  // Deallocate memory for the connection buffer
  HeapFree(GetProcessHeap(), 0, app_data_path);
  app_data_path = NULL;

  return phone_book_path;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasenumconnectionsa
bool DisconnectEntry(const std::wstring& entry_name) {
  DWORD dw_cb = 0;
  DWORD dw_ret = ERROR_SUCCESS;
  DWORD dw_connections = 0;
  LPRASCONN lp_ras_conn = NULL;

  // Call RasEnumConnections with lp_ras_conn = NULL. dw_cb is returned with the
  // required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dw_ret = RasEnumConnections(lp_ras_conn, &dw_cb, &dw_connections);

  if (dw_ret == ERROR_BUFFER_TOO_SMALL) {
    // Allocate the memory needed for the array of RAS structure(s).
    lp_ras_conn = reinterpret_cast<LPRASCONN>(
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dw_cb));
    if (lp_ras_conn == NULL) {
      LOG(ERROR) << "HeapAlloc failed!";
      return false;
    }
    // The first RASCONN structure in the array must contain the RASCONN
    // structure size
    lp_ras_conn[0].dwSize = sizeof(RASCONN);

    // Call RasEnumConnections to enumerate active connections
    dw_ret = RasEnumConnections(lp_ras_conn, &dw_cb, &dw_connections);

    // If successful, print the names of the active connections.
    if (ERROR_SUCCESS == dw_ret) {
      DVLOG(2) << "The following RAS connections are currently active:";
      for (DWORD i = 0; i < dw_connections; i++) {
        std::wstring name(lp_ras_conn[i].szEntryName);
        std::wstring type(lp_ras_conn[i].szDeviceType);
        if (name.compare(entry_name) == 0 && type.compare(L"VPN") == 0) {
          DVLOG(2) << "Disconnect... " << entry_name;
          dw_ret = RasHangUpA(lp_ras_conn[i].hrasconn);
          break;
        }
      }
    }
    // Deallocate memory for the connection buffer
    HeapFree(GetProcessHeap(), 0, lp_ras_conn);
    lp_ras_conn = NULL;
    return dw_ret == ERROR_SUCCESS;
  }

  // There was either a problem with RAS or there are no connections to
  // enumerate
  if (dw_connections >= 1) {
    LOG(ERROR) << "The operation failed to acquire the buffer size.";
    return false;
  }

  DVLOG(2) << "There are no active RAS connections.";
  return true;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasdiala
bool ConnectEntry(const std::wstring& entry_name) {
  LPRASDIALPARAMS lp_ras_dial_params = NULL;
  DWORD cb = sizeof(RASDIALPARAMS);

  lp_ras_dial_params = reinterpret_cast<LPRASDIALPARAMS>(
      HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb));
  if (lp_ras_dial_params == NULL) {
    LOG(ERROR) << "HeapAlloc failed!";
    return false;
  }
  lp_ras_dial_params->dwSize = sizeof(RASDIALPARAMS);
  wcscpy_s(lp_ras_dial_params->szEntryName, RAS_MaxEntryName + 1,
           entry_name.c_str());
  wcscpy_s(lp_ras_dial_params->szDomain, DNLEN + 1, L"*");
  // https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgetcredentialsw
  RASCREDENTIALS credentials;

  ZeroMemory(&credentials, sizeof(RASCREDENTIALS));
  credentials.dwSize = sizeof(RASCREDENTIALS);
  credentials.dwMask = RASCM_UserName | RASCM_Password;
  DWORD dw_ret =
      RasGetCredentials(DEFAULT_PHONE_BOOK, entry_name.c_str(), &credentials);
  if (dw_ret != ERROR_SUCCESS) {
    HeapFree(GetProcessHeap(), 0, (LPVOID)lp_ras_dial_params);
    PrintRasError(dw_ret);
    return false;
  }
  wcscpy_s(lp_ras_dial_params->szUserName, UNLEN + 1, credentials.szUserName);
  wcscpy_s(lp_ras_dial_params->szPassword, PWLEN + 1, credentials.szPassword);

  DVLOG(2) << "Connecting to " << entry_name;
  HRASCONN h_ras_conn = NULL;
  dw_ret = RasDial(NULL, DEFAULT_PHONE_BOOK, lp_ras_dial_params, NULL, NULL,
                   &h_ras_conn);
  if (dw_ret != ERROR_SUCCESS) {
    HeapFree(GetProcessHeap(), 0, (LPVOID)lp_ras_dial_params);
    PrintRasError(dw_ret);
    return false;
  }
  DVLOG(2) << "SUCCESS!";

  HeapFree(GetProcessHeap(), 0, (LPVOID)lp_ras_dial_params);

  return true;
}

bool RemoveEntry(const std::wstring& entry_name) {
  DWORD dw_ret = RasDeleteEntry(DEFAULT_PHONE_BOOK, entry_name.c_str());
  if (dw_ret != ERROR_SUCCESS) {
    PrintRasError(dw_ret);
    return false;
  }
  return true;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rassetentrypropertiesa
bool CreateEntry(const std::wstring& entry_name,
                 const std::wstring& hostname,
                 const std::wstring& username,
                 const std::wstring& password) {
  RASENTRY entry;
  ZeroMemory(&entry, sizeof(RASENTRY));
  // For descriptions of each field (including valid values) see:
  // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/aa377274(v=vs.85)
  entry.dwSize = sizeof(RASENTRY);
  entry.dwfOptions = RASEO_RemoteDefaultGateway | RASEO_RequireEAP |
                     RASEO_PreviewUserPw | RASEO_PreviewDomain |
                     RASEO_ShowDialingProgress;
  wcscpy_s(entry.szLocalPhoneNumber, RAS_MaxPhoneNumber + 1, hostname.c_str());
  entry.dwfNetProtocols = RASNP_Ip | RASNP_Ipv6;
  entry.dwFramingProtocol = RASFP_Ppp;
  wcscpy_s(entry.szDeviceType, RAS_MaxDeviceType + 1, RASDT_Vpn);
  wcscpy_s(entry.szDeviceName, RAS_MaxDeviceName + 1,
           TEXT("WAN Miniport (IKEv2)"));
  entry.dwType = RASET_Vpn;
  entry.dwEncryptionType = ET_Optional;
  entry.dwVpnStrategy = VS_Ikev2Only;
  entry.dwfOptions2 = RASEO2_DontNegotiateMultilink |
                      RASEO2_ReconnectIfDropped |
                      RASEO2_IPv6RemoteDefaultGateway | RASEO2_CacheCredentials;
  entry.dwRedialCount = 3;
  entry.dwRedialPause = 60;

  // this maps to "Type of sign-in info" => "User name and password"
  entry.dwCustomAuthKey = 26;

  DWORD dw_ret = RasSetEntryProperties(DEFAULT_PHONE_BOOK, entry_name.c_str(),
                                       &entry, entry.dwSize, NULL, NULL);
  if (dw_ret != ERROR_SUCCESS) {
    PrintRasError(dw_ret);
    return false;
  }

  if (SetCredentials(entry_name.c_str(), username.c_str(), password.c_str()) !=
      ERROR_SUCCESS) {
    return false;
  }

  // Policy needs to be set, otherwise you'll see an error like this in
  // `eventvwr`:
  // >> The user DESKTOP - DRCJVG6\brian dialed a connection named BRAVEVPN
  // which has failed.The error code returned on failure is 13868.
  //
  // I've found you can set this manually via PowerShell using the
  // `Set-VpnConnectionIPsecConfiguration` cmdlet:
  // https://docs.microsoft.com/en-us/powershell/module/vpnclient/set-vpnconnectionipsecconfiguration?view=windowsserver2019-ps
  //
  // I've used the following parameters via PowerShell:
  // >> AuthenticationTransformConstants: GCMAES256
  // >> CipherTransformConstants : GCMAES256
  // >> DHGroup : ECP384
  // >> IntegrityCheckMethod : SHA256
  // >> PfsGroup : None
  // >> EncryptionMethod : GCMAES256
  //
  // RAS doesn't expose public methods for editing policy. However, the storage
  // is just an INI format file:
  // `%APPDATA%\Microsoft\Network\Connections\Pbk\rasphone.pbk`
  //
  // The variable being set in this file is similar to the structure
  // `ROUTER_CUSTOM_IKEv2_POLICY0` which was part of MPR (Multiprotocol
  // Routing). The DWORDs are written out byte by byte in 02d format as
  // `CustomIPSecPolicies` and `NumCustomPolicy` is always being set to 1.
  //
  // NOTE: *This IKEv2 implementation (due to policy) might only be supported on
  // Windows 8 and above; we need to check that.*
  //

  constexpr wchar_t kNumCustomPolicy[] = L"1";
  constexpr wchar_t kCustomIPSecPolicies[] =
      L"030000000400000002000000050000000200000000000000";
  std::wstring phone_book_path = GetPhonebookPath();
  if (phone_book_path.empty())
    return false;

  // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilestringw
  BOOL wrote_entry =
      WritePrivateProfileString(entry_name.c_str(), L"NumCustomPolicy",
                                kNumCustomPolicy, phone_book_path.c_str());
  if (!wrote_entry) {
    LOG(ERROR)
        << "ERROR: failed to write \"NumCustomPolicy\" field to `rasphone.pbk`";
    RemoveEntry(entry_name);
    return false;
  }

  wrote_entry =
      WritePrivateProfileString(entry_name.c_str(), L"CustomIPSecPolicies",
                                kCustomIPSecPolicies, phone_book_path.c_str());
  if (!wrote_entry) {
    LOG(ERROR) << "ERROR: failed to write \"CustomIPSecPolicies\" field to "
                  "`rasphone.pbk`";
    RemoveEntry(entry_name);
    return false;
  }

  return true;
}

CheckConnectionResult CheckConnection(const std::wstring& entry_name) {
  if (entry_name.empty())
    return CheckConnectionResult::UNKNOWN;

  DWORD dw_cb = 0;
  DWORD dw_ret = dw_cb;
  DWORD dw_connections = 0;
  LPRASCONN lp_ras_conn = NULL;

  // Call RasEnumConnections with lp_ras_conn = NULL. dw_cb is returned with the
  // required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dw_ret = RasEnumConnections(lp_ras_conn, &dw_cb, &dw_connections);

  // If got success here, it means there is no connected vpn entry.
  if (dw_ret == ERROR_SUCCESS) {
    return CheckConnectionResult::NOT_CONNECTED;
  }

  // Abnormal situation.
  if (dw_ret != ERROR_BUFFER_TOO_SMALL)
    return CheckConnectionResult::UNKNOWN;

  // Allocate the memory needed for the array of RAS structure(s).
  lp_ras_conn = (LPRASCONN)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dw_cb);
  if (lp_ras_conn == NULL) {
    LOG(ERROR) << "HeapAlloc failed!";
    return CheckConnectionResult::UNKNOWN;
  }

  // The first RASCONN structure in the array must contain the RASCONN
  // structure size
  lp_ras_conn[0].dwSize = sizeof(RASCONN);

  // Call RasEnumConnections to enumerate active connections
  dw_ret = RasEnumConnections(lp_ras_conn, &dw_cb, &dw_connections);

  if (ERROR_SUCCESS != dw_ret) {
    HeapFree(GetProcessHeap(), 0, lp_ras_conn);
    lp_ras_conn = NULL;
    return CheckConnectionResult::UNKNOWN;
  }

  // If successful, find connection with |entry_name|.
  CheckConnectionResult result = CheckConnectionResult::NOT_CONNECTED;
  for (DWORD i = 0; i < dw_connections; i++) {
    if (entry_name.compare(lp_ras_conn[i].szEntryName) == 0) {
      result = CheckConnectionResult::CONNECTED;
      break;
    }
  }

  HeapFree(GetProcessHeap(), 0, lp_ras_conn);
  lp_ras_conn = NULL;
  return result;
}

}  // namespace internal

}  // namespace brave_vpn
