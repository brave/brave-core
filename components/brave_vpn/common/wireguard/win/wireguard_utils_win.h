/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_WIREGUARD_UTILS_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_WIREGUARD_UTILS_WIN_H_

#include <optional>
#include <string>
#include <tuple>

#include "base/functional/callback.h"
#include "brave/components/brave_vpn/common/wireguard/wireguard_utils.h"
#include "components/version_info/channel.h"

namespace brave_vpn {

namespace wireguard {

bool IsBraveVPNWireguardTunnelServiceRunning(version_info::Channel channel);
bool IsWireguardServiceInstalled(version_info::Channel channel);
void WireguardGenerateKeypair(version_info::Channel channel,
                              WireguardGenerateKeypairCallback callback);
std::optional<std::string> CreateWireguardConfig(
    const std::string& client_private_key,
    const std::string& server_public_key,
    const std::string& vpn_server_hostname,
    const std::string& mapped_ipv4_address,
    const std::string& dns_servers);
void EnableBraveVpnWireguardService(const std::string& config,
                                    version_info::Channel channel,
                                    BooleanCallback callback);
void DisableBraveVpnWireguardService(version_info::Channel channel,
                                     BooleanCallback callback);

void SetWireguardServiceRegisteredForTesting(bool value);
void ShowBraveVpnStatusTrayIcon();

}  // namespace wireguard

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIN_WIREGUARD_UTILS_WIN_H_
