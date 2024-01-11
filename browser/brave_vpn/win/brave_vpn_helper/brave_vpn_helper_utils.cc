// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_utils.h"

#include <windows.h>
#include <winerror.h>
#include <winsvc.h>
#include <winuser.h>

#include <algorithm>
#include <ios>

#include "base/check.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "base/win/windows_types.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/win/scoped_sc_handle.h"
#include "brave/components/brave_vpn/common/win/utils.h"
#include "chrome/install_static/install_modes.h"
#include "chrome/install_static/install_util.h"

namespace brave_vpn {

// The service starts under system user so we save crashes to
// %PROGRAMDATA%\BraveSoftware\{service name}\Crashpad
base::FilePath GetVpnHelperServiceProfileDir() {
  std::wstring program_data =
      install_static::GetEnvironmentString(L"PROGRAMDATA");
  if (program_data.empty()) {
    return base::FilePath();
  }
  return base::FilePath(program_data)
      .Append(install_static::kCompanyPathName)
      .Append(brave_vpn::GetBraveVpnHelperServiceName());
}

bool IsBraveVPNHelperServiceInstalled() {
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
  if (!scm.IsValid()) {
    VLOG(1) << "::OpenSCManager failed. service_name: "
            << brave_vpn::GetBraveVpnHelperServiceName()
            << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }
  ScopedScHandle service(::OpenService(
      scm.Get(), brave_vpn::GetBraveVpnHelperServiceName().c_str(),
      SERVICE_QUERY_STATUS));

  // Service registered and has not exceeded the number of auto-configured
  // restarts.
  return service.IsValid();
}

bool IsNetworkFiltersInstalled() {
  DCHECK(IsBraveVPNHelperServiceInstalled());
  base::win::RegKey service_storage_key(
      HKEY_LOCAL_MACHINE, brave_vpn::kBraveVpnHelperRegistryStoragePath,
      KEY_READ);
  if (!service_storage_key.Valid()) {
    return false;
  }
  DWORD current = -1;
  if (service_storage_key.ReadValueDW(
          brave_vpn::kBraveVpnHelperFiltersInstalledValue, &current) !=
      ERROR_SUCCESS) {
    return false;
  }
  return current > 0;
}

std::wstring GetBraveVPNConnectionName() {
  return base::UTF8ToWide(
      brave_vpn::GetBraveVPNEntryName(install_static::GetChromeChannel()));
}

std::wstring GetBraveVpnHelperServiceDisplayName() {
  static constexpr wchar_t kBraveVpnServiceDisplayName[] = L" Vpn Service";
  return install_static::GetBaseAppName() + kBraveVpnServiceDisplayName;
}

std::wstring GetBraveVpnHelperServiceName() {
  std::wstring name = GetBraveVpnHelperServiceDisplayName();
  name.erase(std::remove_if(name.begin(), name.end(), isspace), name.end());
  return name;
}

}  // namespace brave_vpn
