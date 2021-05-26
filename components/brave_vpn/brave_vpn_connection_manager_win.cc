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

namespace brave_vpn {

namespace {

#define DEFAULT_PHONE_BOOK NULL

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rassetcredentialsa
DWORD SetCredentials(LPCTSTR entry_name, LPCTSTR username, LPCTSTR password) {
  RASCREDENTIALS credentials;

  ZeroMemory(&credentials, sizeof(RASCREDENTIALS));
  credentials.dwSize = sizeof(RASCREDENTIALS);
  credentials.dwMask = RASCM_UserName | RASCM_Password;

  wcscpy_s(credentials.szUserName, 256, username);
  wcscpy_s(credentials.szPassword, 256, password);

  DWORD dwRet =
      RasSetCredentials(DEFAULT_PHONE_BOOK, entry_name, &credentials, FALSE);
  if (dwRet != ERROR_SUCCESS) {
    PrintRasError(dwRet);
    return dwRet;
  }

  return ERROR_SUCCESS;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rassetentrypropertiesa
DWORD CreateEntry(LPCTSTR entry_name,
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

  DWORD dwRet = RasSetEntryProperties(DEFAULT_PHONE_BOOK, entry_name, &entry,
                                      entry.dwSize, NULL, NULL);
  if (dwRet != ERROR_SUCCESS) {
    PrintRasError(dwRet);
    return dwRet;
  }

  dwRet = SetCredentials(entry_name, username, password);

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

  wchar_t NumCustomPolicy[] = L"1";
  wchar_t CustomIPSecPolicies[] =
      L"020000000600000005000000080000000500000000000000";
  wchar_t AppDataPath[1025] = {0};
  wchar_t PhonebookPath[2048] = {0};

  dwRet = ExpandEnvironmentStrings(TEXT("%APPDATA%"), AppDataPath, 1024);
  if (dwRet != ERROR_SUCCESS) {
    PrintRasError(dwRet);
    // TODO(simonhong): handle error here
  }

  swprintf(PhonebookPath, 2048,
           L"%s\\Microsoft\\Network\\Connections\\Pbk\\rasphone.pbk",
           AppDataPath);

  BOOL wrote_entry = WritePrivateProfileString(entry_name, L"NumCustomPolicy",
                                               NumCustomPolicy, PhonebookPath);
  if (!wrote_entry) {
    wprintf(
        L"ERROR: failed to write \"NumCustomPolicy\" field to `rasphone.pbk`");
    // TODO(simonhong): handle error here
  }

  wrote_entry = WritePrivateProfileString(entry_name, L"CustomIPSecPolicies",
                                          CustomIPSecPolicies, PhonebookPath);
  if (!wrote_entry) {
    wprintf(
        L"ERROR: failed to write \"CustomIPSecPolicies\" field to "
        L"`rasphone.pbk`");
    // TODO(simonhong): handle error here
  }

  return ERROR_SUCCESS;
}

DWORD RemoveEntry(LPCTSTR entry_name) {
  DWORD dwRet = RasDeleteEntry(DEFAULT_PHONE_BOOK, entry_name);
  if (dwRet != ERROR_SUCCESS) {
    PrintRasError(dwRet);
    return dwRet;
  }
  return dwRet;
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
  CreateEntry(name.c_str(), host.c_str(), user.c_str(), password.c_str());
  return true;
}

bool BraveVPNConnectionManagerWin::UpdateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  NOTIMPLEMENTED();
  return true;
}

bool BraveVPNConnectionManagerWin::Connect(const std::string& name) {
  NOTIMPLEMENTED();
  return true;
}

bool BraveVPNConnectionManagerWin::Disconnect(const std::string& name) {
  NOTIMPLEMENTED();
  return true;
}

bool BraveVPNConnectionManagerWin::RemoveVPNConnection(
    const std::string& name) {
  const std::wstring w_name = base::UTF8ToWide(name);
  RemoveEntry(w_name.c_str());
  return true;
}

}  // namespace brave_vpn
