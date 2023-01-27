/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/brave_vpn_helper_state.h"

#include <windows.h>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/scoped_sc_handle.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "chrome/install_static/install_util.h"

namespace brave_vpn {

namespace {
// Helper service has 3 fail actions configured to autorestart the service if
// crashed. The check happens before the service started and counter set to 1,
// thus we calculate attempts from 0 -> 2.
const int kHelperServiceFailActionsNumber = 2;

HRESULT HRESULTFromLastError() {
  const auto error_code = ::GetLastError();
  return (error_code != NO_ERROR) ? HRESULT_FROM_WIN32(error_code) : E_FAIL;
}

int GetServiceLaunchCounterValue() {
  base::win::RegKey key(HKEY_LOCAL_MACHINE,
                        brave_vpn::kBraveVpnHelperRegistryStoragePath,
                        KEY_READ);
  if (!key.Valid()) {
    LOG(ERROR) << "Failed to read the successful launch counter";
    return 0;
  }
  DWORD launch = 0;
  key.ReadValueDW(kBraveVpnHelperLaunchCounterValue, &launch);
  return launch;
}

}  // namespace

bool IsBraveVPNHelperServiceLive() {
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
  if (!scm.IsValid()) {
    LOG(ERROR) << "::OpenSCManager failed. service_name: "
               << brave_vpn::GetVpnServiceName() << ", error: " << std::hex
               << HRESULTFromLastError();
    return false;
  }
  ScopedScHandle service(::OpenService(
      scm.Get(), brave_vpn::GetVpnServiceName().c_str(), SERVICE_QUERY_STATUS));

  // Service registered and has not exceeded the number of auto-configured
  // restarts.
  return service.IsValid() &&
         GetServiceLaunchCounterValue() <= kHelperServiceFailActionsNumber;
}

std::wstring GetBraveVPNConnectionName() {
  return base::UTF8ToWide(
      brave_vpn::GetBraveVPNEntryName(install_static::GetChromeChannel()));
}

std::wstring GetVpnServiceName() {
  std::wstring name = GetVpnServiceDisplayName();
  name.erase(std::remove_if(name.begin(), name.end(), isspace), name.end());
  return name;
}

std::wstring GetVpnServiceDisplayName() {
  static constexpr wchar_t kBraveVpnServiceDisplayName[] = L" Vpn Service ";
  return install_static::GetBaseAppName() + kBraveVpnServiceDisplayName;
}

}  // namespace brave_vpn
