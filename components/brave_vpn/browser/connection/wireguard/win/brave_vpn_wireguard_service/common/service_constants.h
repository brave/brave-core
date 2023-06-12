/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_COMMON_SERVICE_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_COMMON_SERVICE_CONSTANTS_H_

#include <guiddef.h>
#include <string>

#include "base/files/file_path.h"
#include "base/version.h"

namespace brave_vpn {

constexpr wchar_t kBraveVpnWireguardServiceExecutable[] =
    L"brave_vpn_wireguard_service.exe";

// Register and configure windows service.
constexpr char kBraveVpnWireguardServiceInstallSwitchName[] = "install";

// Remove config and all stuff related to service.
constexpr char kBraveVpnWireguardServiceUnnstallSwitchName[] = "uninstall";

// Load wireguard binaries and connect to vpn using passed config.
constexpr char kBraveVpnWireguardServiceConnectSwitchName[] = "connect";

// In this mode the service started on user level and expose UI interfaces
// to work with the service for a user.
constexpr char kBraveVpnWireguardServiceInteractiveSwitchName[] = "interactive";

const CLSID& GetBraveVpnWireguardServiceClsid();
const IID& GetBraveVpnWireguardServiceIid();
std::wstring GetBraveVpnWireguardTunnelServiceName();
std::wstring GetBraveVpnWireguardServiceName();
std::wstring GetBraveVpnWireguardServiceDisplayName();
std::wstring GetBraveVpnWireguardServiceRegistryStoragePath();
base::FilePath GetBraveVPNWireguardServiceInstallationPath(
    const base::FilePath& target_path,
    const base::Version& version);
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_COMMON_SERVICE_CONSTANTS_H_
