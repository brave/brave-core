/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_WIREGUARD_TUNNEL_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_WIREGUARD_TUNNEL_SERVICE_H_

#include <optional>
#include <string>

namespace base {
class FilePath;
}  // namespace base

namespace brave_vpn {

namespace wireguard {

// Returns the last known good config with its AllowedIPs rewritten for
// |allow_lan_traffic|. nullopt when there is no readable last used config or it
// already matches, in which case the persisted file can be reused as is.
std::optional<std::string> GetLastUsedConfigIfLanTrafficChanged(
    bool allow_lan_traffic);

// Functions used from BraveWireguardManager to create and launch a new service.
bool LaunchWireguardService(const std::wstring& config);
bool RemoveExistingWireguardService();
bool CreateAndRunBraveWireguardService(const std::wstring& config);

// Functions used inside Wireguard service to setup connection.
int RunWireguardTunnelService(const base::FilePath& config);
bool WireguardGenerateKeypair(std::string* public_key,
                              std::string* private_key);
}  // namespace wireguard

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_WIREGUARD_TUNNEL_SERVICE_H_
