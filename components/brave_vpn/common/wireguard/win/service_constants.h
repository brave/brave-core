/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_SERVICE_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_SERVICE_CONSTANTS_H_

namespace brave_vpn {

constexpr wchar_t kBraveVpnWireguardServiceExecutable[] =
    L"brave_vpn_wireguard_service.exe";

// Registry flag to count service launches for the fallback.
constexpr wchar_t kBraveVpnWireguardCounterOfTunnelUsage[] =
    L"tunnel_launches_counter";

// Register and configure windows service.
constexpr char kBraveVpnWireguardServiceInstallSwitchName[] = "install";

// Remove config and all stuff related to service.
constexpr char kBraveVpnWireguardServiceUnnstallSwitchName[] = "uninstall";

// Load wireguard binaries and connect to vpn using passed config.
constexpr char kBraveVpnWireguardServiceConnectSwitchName[] = "connect";

// In this mode the service started on user level and expose UI interfaces
// to work with the service for a user.
constexpr char kBraveVpnWireguardServiceInteractiveSwitchName[] = "interactive";

// Notifies users about connected state of the vpn using system notifications.
constexpr char kBraveVpnWireguardServiceNotifyConnectedSwitchName[] =
    "notify-connected";

// Notifies users about disconnected state of the vpn using system
// notifications.
constexpr char kBraveVpnWireguardServiceNotifyDisconnectedSwitchName[] =
    "notify-disconnected";
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_SERVICE_CONSTANTS_H_
