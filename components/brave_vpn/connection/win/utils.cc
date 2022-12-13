/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/connection/win/utils.h"

#include <windows.h>

#include <ras.h>
#include <raserror.h>
#include <stdio.h>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"

#define DEFAULT_PHONE_BOOK NULL

namespace brave_vpn {

namespace {

class ScopedHeapAlloc {
 public:
  explicit ScopedHeapAlloc(SIZE_T dw_bytes) {
    lp_alloc_mem_ = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dw_bytes);
  }
  ~ScopedHeapAlloc() {
    if (lp_alloc_mem_)
      HeapFree(GetProcessHeap(), 0, lp_alloc_mem_);
  }
  ScopedHeapAlloc(const ScopedHeapAlloc&) = delete;
  ScopedHeapAlloc& operator=(const ScopedHeapAlloc&) = delete;

  LPVOID lp_alloc_mem() { return lp_alloc_mem_; }

 private:
  LPVOID lp_alloc_mem_ = NULL;
};

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

std::wstring TryGetPhonebookPath(int key, const std::wstring& entry_name) {
  base::FilePath dir;
  if (base::PathService::Get(key, &dir)) {
    dir = dir.Append(L"Microsoft")
              .Append(L"Network")
              .Append(L"Connections")
              .Append(L"Pbk");
    if (base::DirectoryExists(dir)) {
      base::FilePath phone_book_path = dir.Append(L"rasphone.pbk");
      if (base::PathExists(phone_book_path)) {
        // https://learn.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasvalidateentrynamea
        DWORD nRet = RasValidateEntryName(phone_book_path.value().c_str(),
                                          entry_name.c_str());
        switch (nRet) {
          case ERROR_ALREADY_EXISTS:
            VLOG(2) << __func__ << " : phone book found at \""
                    << phone_book_path.value().c_str()
                    << "\" and it contains the entry \"" << entry_name << "\"";
            break;
          case ERROR_SUCCESS:
            VLOG(2) << __func__ << " : phone book found at \""
                    << phone_book_path.value().c_str()
                    << "\" but it does not contain the entry \"" << entry_name
                    << "\"";
            break;
          default:
            VLOG(2) << __func__ << " : phone book found at \""
                    << phone_book_path.value().c_str()
                    << "\" but validation for the entry \"" << entry_name
                    << "\" failed: " << nRet;
            break;
        }
        return phone_book_path.value();
      } else {
        VLOG(2) << __func__ << " : did not find phone book file at \""
                << phone_book_path.value().c_str() << "\"";
      }
    } else {
      VLOG(2) << __func__ << " : did not find phone book directory at \""
              << dir.value().c_str() << "\"";
    }
  }
  return L"";
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

std::wstring GetPhonebookPath(const std::wstring& entry_name) {
  std::wstring path;

  // look initially in %APPDATA%
  path = TryGetPhonebookPath(base::DIR_ROAMING_APP_DATA, entry_name);
  if (!path.empty()) {
    return path;
  }

  // fall back to the %ALLUSERSPROFILE% directory
  path = TryGetPhonebookPath(base::DIR_COMMON_APP_DATA, entry_name);
  if (!path.empty()) {
    return path;
  }

  VLOG(2) << __func__
          << " : did not find phone book file. This is required to add the VPN "
             "entry.";

  return L"";
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasenumconnectionsa
bool DisconnectEntry(const std::wstring& entry_name) {
  auto connection_result = CheckConnection(entry_name);
  if (connection_result == CheckConnectionResult::DISCONNECTING) {
    VLOG(2) << __func__
            << " Don't try to disconnect while brave vpn entry is already in "
               "disconnecting state";
    return true;
  }

  DWORD dw_cb = 0;
  DWORD dw_ret = ERROR_SUCCESS;
  DWORD dw_connections = 0;
  LPRASCONN lp_ras_conn = NULL;

  // Call RasEnumConnections with lp_ras_conn = NULL. dw_cb is returned with the
  // required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dw_ret = RasEnumConnections(lp_ras_conn, &dw_cb, &dw_connections);

  if (dw_ret == ERROR_BUFFER_TOO_SMALL) {
    // Allocate the memory needed for the array of RAS structure(s).
    ScopedHeapAlloc ras_conn(dw_cb);
    lp_ras_conn = reinterpret_cast<LPRASCONN>(ras_conn.lp_alloc_mem());
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
      VLOG(2) << __func__
              << " : The following RAS connections are currently active:"
              << dw_connections;
      for (DWORD i = 0; i < dw_connections; i++) {
        std::wstring name(lp_ras_conn[i].szEntryName);
        std::wstring type(lp_ras_conn[i].szDeviceType);
        VLOG(2) << __func__ << " : " << name << ", " << type;
        if (name.compare(entry_name) == 0 && type.compare(L"VPN") == 0) {
          VLOG(2) << __func__ << " : Disconnect... " << entry_name;
          dw_ret = RasHangUpA(lp_ras_conn[i].hrasconn);
          break;
        }
      }
    }
    return dw_ret == ERROR_SUCCESS;
  }

  // There was either a problem with RAS or there are no connections to
  // enumerate
  if (dw_connections >= 1) {
    LOG(ERROR) << "The operation failed to acquire the buffer size.";
    return false;
  }

  VLOG(2) << "There are no active RAS connections.";
  return true;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasdiala
bool ConnectEntry(const std::wstring& entry_name) {
  auto connection_result = CheckConnection(entry_name);
  if (connection_result == CheckConnectionResult::CONNECTING ||
      connection_result == CheckConnectionResult::CONNECTED) {
    VLOG(2)
        << __func__
        << " Don't try to connect when it's in-progress or already connected.";
    return true;
  }

  LPRASDIALPARAMS lp_ras_dial_params = NULL;
  DWORD cb = sizeof(RASDIALPARAMS);

  ScopedHeapAlloc ras_dial_params(cb);
  lp_ras_dial_params =
      reinterpret_cast<LPRASDIALPARAMS>(ras_dial_params.lp_alloc_mem());
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
    PrintRasError(dw_ret);
    return false;
  }
  wcscpy_s(lp_ras_dial_params->szUserName, UNLEN + 1, credentials.szUserName);
  wcscpy_s(lp_ras_dial_params->szPassword, PWLEN + 1, credentials.szPassword);

  VLOG(2) << __func__ << " : Connecting to " << entry_name;
  HRASCONN h_ras_conn = NULL;
  dw_ret = RasDial(NULL, DEFAULT_PHONE_BOOK, lp_ras_dial_params, 0, NULL,
                   &h_ras_conn);

  if (dw_ret == ERROR_DIAL_ALREADY_IN_PROGRESS) {
    // We should not treat this as failure state.
    // Just return when already dialed.
    PrintRasError(dw_ret);
    return true;
  }

  if (dw_ret != ERROR_SUCCESS) {
    PrintRasError(dw_ret);

    // To clear state.
    VLOG(2) << __func__ << ": RasDial() failed. Try RasHangUp() to clear state";
    if (dw_ret = RasHangUp(h_ras_conn); dw_ret != ERROR_SUCCESS)
      PrintRasError(dw_ret);

    return false;
  }

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
  auto connection_result = CheckConnection(entry_name);
  if (connection_result == CheckConnectionResult::CONNECTING ||
      connection_result == CheckConnectionResult::CONNECTED) {
    VLOG(2) << __func__
            << " Don't try to create entry when brave vpn entry is in "
               "connecting or connected state";
    return true;
  }

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
  std::wstring phone_book_path = GetPhonebookPath(entry_name);
  if (phone_book_path.empty())
    return false;

  // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilestringw
  BOOL wrote_entry =
      WritePrivateProfileString(entry_name.c_str(), L"NumCustomPolicy",
                                kNumCustomPolicy, phone_book_path.c_str());
  if (!wrote_entry) {
    LOG(ERROR)
        << "ERROR: failed to write \"NumCustomPolicy\" field to `rasphone.pbk`";
    return false;
  }

  wrote_entry =
      WritePrivateProfileString(entry_name.c_str(), L"CustomIPSecPolicies",
                                kCustomIPSecPolicies, phone_book_path.c_str());
  if (!wrote_entry) {
    LOG(ERROR) << "ERROR: failed to write \"CustomIPSecPolicies\" field to "
                  "`rasphone.pbk`";
    return false;
  }

  return true;
}

CheckConnectionResult GetConnectionState(HRASCONN h_ras_conn) {
  DWORD dw_ret = 0;

  RASCONNSTATUS ras_conn_status;
  ZeroMemory(&ras_conn_status, sizeof(RASCONNSTATUS));
  ras_conn_status.dwSize = sizeof(RASCONNSTATUS);

  // Checking connection status using RasGetConnectStatus
  dw_ret = RasGetConnectStatus(h_ras_conn, &ras_conn_status);
  if (ERROR_SUCCESS != dw_ret) {
    LOG(ERROR) << "RasGetConnectStatus failed: Error = " << dw_ret;
    return CheckConnectionResult::DISCONNECTED;
  }

  switch (ras_conn_status.rasconnstate) {
    case RASCS_ConnectDevice:
      VLOG(2) << "Connecting device...";
      return CheckConnectionResult::CONNECTING;
    case RASCS_Connected:
      VLOG(2) << "Connected";
      return CheckConnectionResult::CONNECTED;
    case RASCS_Disconnected:
      VLOG(2) << "Disconnected";
      return CheckConnectionResult::DISCONNECTED;
    default:
      break;
  }

  return CheckConnectionResult::DISCONNECTED;
}

CheckConnectionResult CheckConnection(const std::wstring& entry_name) {
  VLOG(2) << "Check connection state for " << entry_name;
  if (entry_name.empty())
    return CheckConnectionResult::DISCONNECTED;

  DWORD dw_cb = 0;
  DWORD dw_ret = dw_cb;
  DWORD dw_connections = 0;
  LPRASCONN lp_ras_conn = NULL;

  // Call RasEnumConnections with lp_ras_conn = NULL. dw_cb is returned with the
  // required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dw_ret = RasEnumConnections(lp_ras_conn, &dw_cb, &dw_connections);

  // If got success here, it means there is no connected vpn entry.
  if (dw_ret == ERROR_SUCCESS) {
    VLOG(2) << " There is no active connections.";
    return CheckConnectionResult::DISCONNECTED;
  }

  // Abnormal situation.
  if (dw_ret != ERROR_BUFFER_TOO_SMALL)
    return CheckConnectionResult::DISCONNECTED;

  // Allocate the memory needed for the array of RAS structure(s).
  ScopedHeapAlloc ras_conn(dw_cb);
  lp_ras_conn = reinterpret_cast<LPRASCONN>(ras_conn.lp_alloc_mem());
  if (lp_ras_conn == NULL) {
    LOG(ERROR) << "HeapAlloc failed!";
    return CheckConnectionResult::DISCONNECTED;
  }

  // The first RASCONN structure in the array must contain the RASCONN
  // structure size
  lp_ras_conn[0].dwSize = sizeof(RASCONN);

  // Call RasEnumConnections to enumerate active connections
  dw_ret = RasEnumConnections(lp_ras_conn, &dw_cb, &dw_connections);

  if (ERROR_SUCCESS != dw_ret) {
    lp_ras_conn = NULL;
    return CheckConnectionResult::DISCONNECTED;
  }

  // If successful, find connection with |entry_name|.
  CheckConnectionResult result = CheckConnectionResult::DISCONNECTED;
  for (DWORD i = 0; i < dw_connections; i++) {
    if (entry_name.compare(lp_ras_conn[i].szEntryName) == 0) {
      result = GetConnectionState(lp_ras_conn[i].hrasconn);
      break;
    }
  }
  return result;
}

}  // namespace internal

}  // namespace brave_vpn
