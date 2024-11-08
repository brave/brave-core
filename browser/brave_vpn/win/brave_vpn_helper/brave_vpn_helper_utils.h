/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_UTILS_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_UTILS_H_

#include <string>

#include "base/files/file_path.h"

namespace brave_vpn {

base::FilePath GetVpnHelperServiceProfileDir();
bool IsBraveVPNHelperServiceInstalled();
bool IsNetworkFiltersInstalled();
std::wstring GetBraveVPNConnectionName();
std::wstring GetBraveVpnHelperServiceDisplayName();
std::wstring GetBraveVpnHelperServiceName();
std::wstring GetBraveVpnHelperServiceDescription();
std::wstring GetBraveVpnHelperRegistryStoragePath();
std::wstring GetBraveVpnOneTimeServiceCleanupStoragePath();

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_UTILS_H_
