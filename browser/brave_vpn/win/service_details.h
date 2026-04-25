/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_SERVICE_DETAILS_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_SERVICE_DETAILS_H_

#include <guiddef.h>
#include <string>

#include "base/files/file_path.h"
#include "base/version.h"

namespace brave_vpn {
const CLSID& GetBraveVpnWireguardServiceClsid();
const IID& GetBraveVpnWireguardServiceIid();
std::wstring GetBraveVpnWireguardTunnelServiceName();
std::wstring GetBraveVpnWireguardServiceName();
std::wstring GetBraveVpnWireguardServiceDisplayName();
std::wstring GetBraveVpnWireguardServiceDescription();
base::FilePath GetBraveVPNWireguardServiceExecutablePath(
    const base::FilePath& root_dir);
}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_SERVICE_DETAILS_H_
