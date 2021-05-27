/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_connection_manager_win.h"

#include <windows.h>

#include <MprApi.h>
#include <ipsectypes.h>
#include <ras.h>
#include <raserror.h>
#include <stdio.h>
#include <winerror.h>
#include <winsock.h>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/utils_win.h"

// Most of Windows implementations are based on Brian Clifton
// (brian@clifton.me)'s work (https://github.com/bsclifton/winvpntool).

namespace brave_vpn {

namespace {

#define DEFAULT_PHONE_BOOK NULL

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasdiala
bool ConnectEntry(LPCTSTR entry_name) {
  LPRASDIALPARAMS lp_ras_dial_params = NULL;
  DWORD cb = sizeof(RASDIALPARAMS);

  lp_ras_dial_params =
      (LPRASDIALPARAMS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
  if (lp_ras_dial_params == NULL) {
    wprintf(L"HeapAlloc failed!\n");
    return false;
  }
  lp_ras_dial_params->dwSize = sizeof(RASDIALPARAMS);
  wcscpy_s(lp_ras_dial_params->szEntryName, 256, entry_name);
  wcscpy_s(lp_ras_dial_params->szDomain, 15, L"*");
  // https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgetcredentialsw
  RASCREDENTIALS credentials;

  ZeroMemory(&credentials, sizeof(RASCREDENTIALS));
  credentials.dwSize = sizeof(RASCREDENTIALS);
  credentials.dwMask = RASCM_UserName | RASCM_Password;
  DWORD dw_ret =
      RasGetCredentials(DEFAULT_PHONE_BOOK, entry_name, &credentials);
  if (dw_ret != ERROR_SUCCESS) {
    HeapFree(GetProcessHeap(), 0, (LPVOID)lp_ras_dial_params);
    PrintRasError(dw_ret);
    return false;
  }
  wcscpy_s(lp_ras_dial_params->szUserName, 256, credentials.szUserName);
  wcscpy_s(lp_ras_dial_params->szPassword, 256, credentials.szPassword);

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

  return ERROR_SUCCESS;
}

bool RemoveEntry(LPCTSTR entry_name) {
  DWORD dw_ret = RasDeleteEntry(DEFAULT_PHONE_BOOK, entry_name);
  if (dw_ret != ERROR_SUCCESS) {
    PrintRasError(dw_ret);
    return false;
  }
  return true;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rassetcredentialsa
DWORD SetCredentials(LPCTSTR entry_name, LPCTSTR username, LPCTSTR password) {
  RASCREDENTIALS credentials;

  ZeroMemory(&credentials, sizeof(RASCREDENTIALS));
  credentials.dwSize = sizeof(RASCREDENTIALS);
  credentials.dwMask = RASCM_UserName | RASCM_Password;

  wcscpy_s(credentials.szUserName, 256, username);
  wcscpy_s(credentials.szPassword, 256, password);

  DWORD dw_ret =
      RasSetCredentials(DEFAULT_PHONE_BOOK, entry_name, &credentials, FALSE);
  if (dw_ret != ERROR_SUCCESS) {
    PrintRasError(dw_ret);
    return dw_ret;
  }

  return ERROR_SUCCESS;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rassetentrypropertiesa
bool CreateEntry(LPCTSTR entry_name,
                 LPCTSTR hostname,
                 LPCTSTR username,
                 LPCTSTR password) {
  RASENTRY entry;
  ZeroMemory(&entry, sizeof(RASENTRY));
  // For descriptions of each field (including valid values) see:
  // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/aa377274(v=vs.85)
  entry.dwSize = sizeof(RASENTRY);
  entry.dwfOptions = RASEO_RemoteDefaultGateway | RASEO_RequireEAP |
                     RASEO_PreviewUserPw | RASEO_PreviewDomain |
                     RASEO_ShowDialingProgress;
  wcscpy_s(entry.szLocalPhoneNumber, 128, hostname);
  entry.dwfNetProtocols = RASNP_Ip | RASNP_Ipv6;
  entry.dwFramingProtocol = RASFP_Ppp;
  wcscpy_s(entry.szDeviceType, 16, RASDT_Vpn);
  wcscpy_s(entry.szDeviceName, 128, TEXT("WAN Miniport (IKEv2)"));
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

  DWORD dw_ret = RasSetEntryProperties(DEFAULT_PHONE_BOOK, entry_name, &entry,
                                       entry.dwSize, NULL, NULL);
  if (dw_ret != ERROR_SUCCESS) {
    PrintRasError(dw_ret);
    return false;
  }

  if (SetCredentials(entry_name, username, password) != ERROR_SUCCESS) {
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
      L"020000000600000005000000080000000500000000000000";
  std::wstring phone_book_path = GetPhonebookPath();
  if (phone_book_path.empty())
    return false;

  // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilestringw
  BOOL wrote_entry =
      WritePrivateProfileString(entry_name, L"NumCustomPolicy",
                                kNumCustomPolicy, phone_book_path.c_str());
  if (!wrote_entry) {
    LOG(ERROR)
        << "ERROR: failed to write \"NumCustomPolicy\" field to `rasphone.pbk`";
    RemoveEntry(entry_name);
    return false;
  }

  wrote_entry =
      WritePrivateProfileString(entry_name, L"CustomIPSecPolicies",
                                kCustomIPSecPolicies, phone_book_path.c_str());
  if (!wrote_entry) {
    LOG(ERROR) << "ERROR: failed to write \"CustomIPSecPolicies\" field to "
                  "`rasphone.pbk`";
    RemoveEntry(entry_name);
    return false;
  }

  return true;
}

}  // namespace

// static
BraveVPNConnectionManager* BraveVPNConnectionManager::GetInstance() {
  static base::NoDestructor<BraveVPNConnectionManagerWin> s_manager;
  return s_manager.get();
}

BraveVPNConnectionManagerWin::BraveVPNConnectionManagerWin() = default;
BraveVPNConnectionManagerWin::~BraveVPNConnectionManagerWin() = default;

bool BraveVPNConnectionManagerWin::CreateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  const std::wstring name = base::UTF8ToWide(info.name);
  const std::wstring host = base::UTF8ToWide(info.url);
  const std::wstring user = base::UTF8ToWide(info.id);
  const std::wstring password = base::UTF8ToWide(info.pwd);
  return CreateEntry(name.c_str(), host.c_str(), user.c_str(),
                     password.c_str());
}

bool BraveVPNConnectionManagerWin::UpdateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  NOTIMPLEMENTED();
  return true;
}

bool BraveVPNConnectionManagerWin::Connect(const std::string& name) {
  const std::wstring w_name = base::UTF8ToWide(name);
  ConnectEntry(w_name.c_str());
  return true;
}

bool BraveVPNConnectionManagerWin::Disconnect(const std::string& name) {
  NOTIMPLEMENTED();
  return true;
}

bool BraveVPNConnectionManagerWin::RemoveVPNConnection(
    const std::string& name) {
  const std::wstring w_name = base::UTF8ToWide(name);
  return RemoveEntry(w_name.c_str());
}

}  // namespace brave_vpn
