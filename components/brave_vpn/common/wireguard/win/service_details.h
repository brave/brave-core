/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_SERVICE_DETAILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_SERVICE_DETAILS_H_

#include <guiddef.h>
#include <string>

#include "base/files/file_path.h"
#include "base/version.h"
#include "components/version_info/channel.h"

namespace brave_vpn {
const CLSID& GetBraveVpnWireguardServiceClsid(version_info::Channel channel);
const IID& GetBraveVpnWireguardServiceIid();
std::wstring GetBraveVpnWireguardTunnelServiceName(
    version_info::Channel channel);
std::wstring GetBraveVpnWireguardServiceName(version_info::Channel channel);
std::wstring GetBraveVpnWireguardServiceDisplayName(
    version_info::Channel channel);
base::FilePath GetBraveVPNWireguardServiceInstallationPath(
    const base::FilePath& target_path,
    const base::Version& version);
base::FilePath GetBraveVPNWireguardServiceExecutablePath();
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_SERVICE_DETAILS_H_
