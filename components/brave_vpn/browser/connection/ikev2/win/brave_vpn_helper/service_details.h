/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_BRAVE_VPN_HELPER_SERVICE_DETAILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_BRAVE_VPN_HELPER_SERVICE_DETAILS_H_

#include <string>

#include "base/files/file_path.h"
#include "base/version.h"

namespace brave_vpn {
bool IsBraveVPNHelperServiceInstalled();
bool IsNetworkFiltersInstalled();
std::wstring GetBraveVPNConnectionName();
std::wstring GetBraveVpnHelperServiceName();
std::wstring GetBraveVpnHelperServiceDisplayName();
base::FilePath GetVpnHelperServiceProfileDir();
base::FilePath GetBraveVpnHelperServicePath(const base::FilePath& target_path,
                                            const base::Version& version);
base::FilePath GetBraveVpnHelperServicePath();
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_BRAVE_VPN_HELPER_SERVICE_DETAILS_H_
