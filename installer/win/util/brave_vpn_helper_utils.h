/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_INSTALLER_WIN_UTIL_BRAVE_VPN_HELPER_UTILS_H_
#define BRAVE_INSTALLER_WIN_UTIL_BRAVE_VPN_HELPER_UTILS_H_

#include <windows.h>
#include <string>

#include "base/files/file_path.h"
#include "base/version.h"

namespace brave_vpn {

bool ConfigureServiceAutoRestart(const std::wstring& service_name,
                                 const std::wstring& brave_vpn_entry);
base::FilePath GetBraveVpnHelperServicePath();
base::FilePath GetBraveVpnHelperServicePath(const base::FilePath& target_path,
                                            const base::Version& version);
base::FilePath GetVpnHelperServiceProfileDir();
bool InstallBraveVPNHelperService();
bool IsBraveVPNHelperServiceInstalled();
bool IsNetworkFiltersInstalled();
HRESULT InstallBraveVPNHelperServiceImpersonated();
HRESULT InstallBraveWireGuardServiceImpersonated();
std::wstring GetBraveVPNConnectionName();
std::wstring GetBraveVpnHelperServiceDisplayName();
std::wstring GetBraveVpnHelperServiceName();

bool InstallVPNSystemServices();

}  // namespace brave_vpn

#endif  // BRAVE_INSTALLER_WIN_UTIL_BRAVE_VPN_HELPER_UTILS_H_
