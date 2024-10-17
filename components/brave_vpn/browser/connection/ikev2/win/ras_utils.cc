/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/brave_vpn/browser/connection/ikev2/win/ras_utils.h"

#include <windows.h>
#include <ras.h>
#include <raserror.h>
#include <stdio.h>

#include <optional>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/base/process/process_launcher.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"

namespace brave_vpn {

namespace {

class ScopedHeapAlloc {
 public:
  explicit ScopedHeapAlloc(SIZE_T dw_bytes) {
    lp_alloc_mem_ = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dw_bytes);
  }
  ~ScopedHeapAlloc() {
    if (lp_alloc_mem_) {
      HeapFree(GetProcessHeap(), 0, lp_alloc_mem_);
    }
  }
  ScopedHeapAlloc(const ScopedHeapAlloc&) = delete;
  ScopedHeapAlloc& operator=(const ScopedHeapAlloc&) = delete;

  LPVOID lp_alloc_mem() { return lp_alloc_mem_; }

 private:
  LPVOID lp_alloc_mem_ = NULL;
};

// https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
std::string GetSystemError(DWORD error) {
  constexpr DWORD kBufSize = 512;
  TCHAR lpsz_error_string[kBufSize];

  DWORD buf_len =
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    lpsz_error_string, kBufSize, NULL);
  if (!buf_len) {
    return "";
  }

  LOG(ERROR) << lpsz_error_string;
  return base::WideToUTF8(lpsz_error_string);
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rassetcredentialsa
std::optional<std::string> SetCredentials(LPCTSTR phone_book_path,
                                          LPCTSTR entry_name,
                                          LPCTSTR username,
                                          LPCTSTR password) {
  RASCREDENTIALS credentials;

  ZeroMemory(&credentials, sizeof(RASCREDENTIALS));
  credentials.dwSize = sizeof(RASCREDENTIALS);
  credentials.dwMask = RASCM_UserName | RASCM_Password;

  wcscpy_s(credentials.szUserName, UNLEN + 1, username);
  wcscpy_s(credentials.szPassword, PWLEN + 1, password);

  DWORD dw_ret =
      RasSetCredentials(phone_book_path, entry_name, &credentials, FALSE);
  if (dw_ret != ERROR_SUCCESS) {
    return base::StrCat(
        {"RasSetCredential() - ", ras::GetRasErrorMessage(dw_ret)});
  }

  return std::nullopt;
}

std::optional<std::wstring> TryToCreateEmptyPhoneBookFile() {
  base::FilePath dir;
  if (!base::PathService::Get(base::DIR_ROAMING_APP_DATA, &dir)) {
    return std::nullopt;
  }

  dir = dir.Append(L"Microsoft")
            .Append(L"Network")
            .Append(L"Connections")
            .Append(L"Pbk");
  if (!base::CreateDirectoryAndGetError(dir, nullptr)) {
    return std::nullopt;
  }

  base::FilePath phone_book_path = dir.Append(L"rasphone.pbk");
  base::ScopedFILE file_stream(base::OpenFile(phone_book_path, "w"));
  if (file_stream) {
    return phone_book_path.value();
  }
  return std::nullopt;
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

ras::RasOperationResult GetRasSuccessResult() {
  ras::RasOperationResult result;
  result.success = true;
  return result;
}

ras::RasOperationResult GetRasErrorResult(const std::string& error,
                                          const std::string& caller = {}) {
  ras::RasOperationResult result;
  result.success = false;
  result.error_description =
      caller.empty() ? error : base::StrCat({caller, " - ", error});
  return result;
}

}  // namespace

namespace ras {

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgeterrorstringa
std::string GetRasErrorMessage(DWORD error) {
  constexpr DWORD kBufSize = 512;
  TCHAR lpsz_error_string[kBufSize];

  if (error > RASBASE && error < RASBASEEND) {
    if (RasGetErrorString(error, lpsz_error_string, kBufSize) ==
        ERROR_SUCCESS) {
      return base::WideToUTF8(lpsz_error_string);
    }
  }

  return GetSystemError(error);
}

std::wstring GetPhonebookPath(const std::wstring& entry_name,
                              std::string* error) {
  std::wstring path;

  // look initially in %APPDATA%
  path = TryGetPhonebookPath(base::DIR_ROAMING_APP_DATA, entry_name);
  if (!path.empty()) {
    return path;
  }
  *error = "failed to get phonebook path from APPDATA";

  // fall back to the %ALLUSERSPROFILE% directory
  path = TryGetPhonebookPath(base::DIR_COMMON_APP_DATA, entry_name);
  if (!path.empty()) {
    *error = "";
    return path;
  }

  *error = "failed to get phonebook path from ALLUSERSPROFILE and APPDATA";
  VLOG(2) << __func__
          << " : did not find phone book file. This is required to add the VPN "
             "entry. Try to create empty pbk file instead.";

  return TryToCreateEmptyPhoneBookFile().value_or(L"");
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasenumconnectionsa
RasOperationResult DisconnectEntry(const std::wstring& entry_name) {
  auto connection_result = CheckConnection(entry_name);
  if (connection_result == CheckConnectionResult::DISCONNECTING) {
    VLOG(2) << __func__
            << " Don't try to disconnect while brave vpn entry is already in "
               "disconnecting state";
    return GetRasSuccessResult();
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
      return GetRasErrorResult("HeapAlloc failed at DisconnectEntry()");
    }
    // The first RASCONN structure in the array must contain the RASCONN
    // structure size
    lp_ras_conn[0].dwSize = sizeof(RASCONN);

    // Call RasEnumConnections to enumerate active connections
    dw_ret = RasEnumConnections(lp_ras_conn, &dw_cb, &dw_connections);
    std::string caller = "RasEnumConnection()";

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
          dw_ret = RasHangUp(lp_ras_conn[i].hrasconn);
          if (dw_ret != ERROR_SUCCESS) {
            caller = "RasHangUp()";
          }
          break;
        }
      }
    }

    if (dw_ret == ERROR_SUCCESS) {
      return GetRasSuccessResult();
    }

    return GetRasErrorResult(GetRasErrorMessage(dw_ret), caller);
  }

  // There was either a problem with RAS or there are no connections to
  // enumerate
  if (dw_connections >= 1) {
    return GetRasErrorResult(
        "The operation failed to acquire the buffer size at "
        "DisconnectEntry().");
  }

  VLOG(2) << "There are no active RAS connections.";
  return GetRasSuccessResult();
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasdiala
RasOperationResult ConnectEntry(const std::wstring& entry_name) {
  auto connection_result = CheckConnection(entry_name);
  if (connection_result == CheckConnectionResult::CONNECTING ||
      connection_result == CheckConnectionResult::CONNECTED) {
    VLOG(2)
        << __func__
        << " Don't try to connect when it's in-progress or already connected.";
    return GetRasSuccessResult();
  }

  std::string error_get_phone_book_path;
  const std::wstring phone_book_path =
      GetPhonebookPath(entry_name, &error_get_phone_book_path);
  if (phone_book_path.empty()) {
    return GetRasErrorResult(error_get_phone_book_path,
                             "GetPhonebookPath() from ConnectEntry()");
  }

  LPRASDIALPARAMS lp_ras_dial_params = NULL;
  DWORD cb = sizeof(RASDIALPARAMS);

  ScopedHeapAlloc ras_dial_params(cb);
  lp_ras_dial_params =
      reinterpret_cast<LPRASDIALPARAMS>(ras_dial_params.lp_alloc_mem());
  if (lp_ras_dial_params == NULL) {
    return GetRasErrorResult("HeapAlloc failed at ConnectEntry().");
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
  DWORD dw_ret = RasGetCredentials(phone_book_path.c_str(), entry_name.c_str(),
                                   &credentials);
  if (dw_ret != ERROR_SUCCESS) {
    return GetRasErrorResult(GetRasErrorMessage(dw_ret), "RasGetCredentials()");
  }
  wcscpy_s(lp_ras_dial_params->szUserName, UNLEN + 1, credentials.szUserName);
  wcscpy_s(lp_ras_dial_params->szPassword, PWLEN + 1, credentials.szPassword);

  VLOG(2) << __func__ << " : Connecting to " << entry_name;
  HRASCONN h_ras_conn = NULL;
  dw_ret = RasDial(NULL, phone_book_path.c_str(), lp_ras_dial_params, 0, NULL,
                   &h_ras_conn);

  if (dw_ret == ERROR_DIAL_ALREADY_IN_PROGRESS) {
    // We should not treat this as failure state.
    // Just return when already dialed.
    VLOG(2) << __func__ << GetRasErrorMessage(dw_ret);
    return GetRasSuccessResult();
  }

  if (dw_ret != ERROR_SUCCESS) {
    auto result = GetRasErrorResult(GetRasErrorMessage(dw_ret), "RasDial()");

    // To clear state.
    VLOG(2) << __func__ << ": RasDial() failed. Try RasHangUp() to clear state";
    if (dw_ret = RasHangUp(h_ras_conn); dw_ret != ERROR_SUCCESS) {
      result.error_description =
          base::StrCat({result.error_description, ", ", "RasHangUp() - ",
                        GetRasErrorMessage(dw_ret)});
    }

    return result;
  }

  return GetRasSuccessResult();
}

RasOperationResult RemoveEntry(const std::wstring& entry_name) {
  std::string error_get_phone_book_path;
  const std::wstring phone_book_path =
      GetPhonebookPath(entry_name, &error_get_phone_book_path);
  if (phone_book_path.empty()) {
    return GetRasErrorResult(error_get_phone_book_path,
                             "GetPhonebookPath() from RemoveEntry()");
  }
  auto disconnected = DisconnectEntry(entry_name);
  if (!disconnected.success) {
    VLOG(2) << __func__ << ": Unable to disconnect " << entry_name
            << ", error:" << disconnected.error_description;
  }
  DWORD dw_ret = RasDeleteEntry(phone_book_path.c_str(), entry_name.c_str());
  if (dw_ret != ERROR_SUCCESS) {
    return GetRasErrorResult(GetRasErrorMessage(dw_ret), "RasDeleteEntry()");
  }

  return GetRasSuccessResult();
}

// `Set-VpnConnectionIPsecConfiguration` cmdlet:
// https://docs.microsoft.com/en-us/powershell/module/vpnclient/set-vpnconnectionipsecconfiguration?view=windowsserver2019-ps
RasOperationResult SetConnectionParamsUsingPowerShell(
    const std::wstring& entry_name) {
  base::CommandLine power_shell(base::FilePath(L"PowerShell"));
  power_shell.AppendArg("Set-VpnConnectionIPsecConfiguration");
  power_shell.AppendArg("-ConnectionName");
  power_shell.AppendArg(base::WideToUTF8(entry_name));
  power_shell.AppendArg("-AuthenticationTransformConstants");
  power_shell.AppendArg("SHA256128");
  power_shell.AppendArg("-CipherTransformConstants");
  power_shell.AppendArg("AES256");
  power_shell.AppendArg("-DHGroup");
  power_shell.AppendArg("Group2");
  power_shell.AppendArg("-IntegrityCheckMethod");
  power_shell.AppendArg("SHA384");
  power_shell.AppendArg("-PfsGroup");
  power_shell.AppendArg("None");
  power_shell.AppendArg("-EncryptionMethod");
  power_shell.AppendArg("AES256");
  power_shell.AppendArg("-Force");
  base::LaunchOptions options;
  options.start_hidden = true;
  auto result = brave::ProcessLauncher::ReadAppOutput(power_shell, options, 10);
  if (!result.has_value()) {
    return GetRasErrorResult(logging::SystemErrorCodeToString(GetLastError()));
  }
  return GetRasSuccessResult();
}

RasOperationResult SetConnectionParamsWin32(
    const std::wstring& entry_name,
    const std::wstring& phone_book_path) {
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
  // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilestringw
  BOOL wrote_entry =
      WritePrivateProfileString(entry_name.c_str(), L"NumCustomPolicy",
                                kNumCustomPolicy, phone_book_path.c_str());
  if (!wrote_entry) {
    return GetRasErrorResult(
        "failed to write \"NumCustomPolicy\" field to `rasphone.pbk`");
  }

  wrote_entry =
      WritePrivateProfileString(entry_name.c_str(), L"CustomIPSecPolicies",
                                kCustomIPSecPolicies, phone_book_path.c_str());
  if (!wrote_entry) {
    return GetRasErrorResult(
        "failed to write \"CustomIPSecPolicies\" field to `rasphone.pbk`");
  }
  return GetRasSuccessResult();
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rassetentrypropertiesa
RasOperationResult CreateEntry(const BraveVPNConnectionInfo& info) {
  const auto entry_name = base::UTF8ToWide(info.connection_name());
  const auto hostname = base::UTF8ToWide(info.hostname());
  const auto username = base::UTF8ToWide(info.username());
  const auto password = base::UTF8ToWide(info.password());

  // `RasSetEntryProperties` can have problems if fields are empty.
  // Specifically, it will crash if `hostname` is NULL. Entry name
  // is already validated.
  if (hostname.empty()) {
    VLOG(2) << __func__ << " Can't create entry with empty `hostname`";
    return GetRasErrorResult("`hostname` is empty");
  }

  std::string error_get_phone_book_path;
  const std::wstring phone_book_path =
      GetPhonebookPath(entry_name, &error_get_phone_book_path);
  if (phone_book_path.empty()) {
    return GetRasErrorResult(error_get_phone_book_path,
                             "GetPhonebookPath() from CreateEntry()");
  }

  auto connection_result = CheckConnection(entry_name);
  if (connection_result == CheckConnectionResult::CONNECTING ||
      connection_result == CheckConnectionResult::CONNECTED) {
    VLOG(2) << __func__
            << " Don't try to create entry when brave vpn entry is in "
               "connecting or connected state";
    return GetRasSuccessResult();
  }

  VLOG(2) << __func__ << " Create Entry(" << entry_name << ") with "
          << hostname;

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

  DWORD dw_ret =
      RasSetEntryProperties(phone_book_path.c_str(), entry_name.c_str(), &entry,
                            entry.dwSize, NULL, NULL);
  if (dw_ret != ERROR_SUCCESS) {
    return GetRasErrorResult(GetRasErrorMessage(dw_ret),
                             "RasSetEntryProperties()");
  }

  if (const auto error =
          SetCredentials(phone_book_path.c_str(), entry_name.c_str(),
                         username.c_str(), password.c_str())) {
    return GetRasErrorResult(*error);
  }

  // Policy needs to be set, otherwise you'll see an error like this in
  // `eventvwr`:
  // >> The user DESKTOP - DRCJVG6\brian dialed a connection named BRAVEVPN
  // which has failed.The error code returned on failure is 13868.
  //
  if (!SetConnectionParamsUsingPowerShell(entry_name).success) {
    return SetConnectionParamsWin32(entry_name, phone_book_path);
  }
  return GetRasSuccessResult();
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
  if (entry_name.empty()) {
    return CheckConnectionResult::DISCONNECTED;
  }

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
  if (dw_ret != ERROR_BUFFER_TOO_SMALL) {
    return CheckConnectionResult::DISCONNECTED;
  }

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

}  // namespace ras

}  // namespace brave_vpn
