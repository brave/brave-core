/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIREGUARD_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIREGUARD_UTILS_H_

#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "base/functional/callback.h"

namespace brave_vpn {

namespace wireguard {

using BooleanCallback = base::OnceCallback<void(bool)>;
using WireguardKeyPair = std::optional<std::tuple<std::string, std::string>>;
using WireguardGenerateKeypairCallback =
    base::OnceCallback<void(WireguardKeyPair)>;

std::optional<std::string> CreateWireguardConfig(
    const std::string& client_private_key,
    const std::string& server_public_key,
    const std::string& vpn_server_hostname,
    const std::string& mapped_ipv4_address,
    bool allow_lan_traffic);

WireguardKeyPair GenerateNewX25519Keypair();

std::vector<std::string> ParseAllowedIPs(const std::string& config);

// Rewrites the AllowedIPs line of an existing config so it matches
// |allow_lan_traffic|, leaving the rest of the config untouched. Used to
// refresh a persisted config whose server details we can no longer reproduce.
// Returns nullopt when the config has no AllowedIPs line.
std::optional<std::string> UpdateAllowedIPs(const std::string& config,
                                            bool allow_lan_traffic);

// Returns true when AllowedIPs uses literal /0 prefixes (full tunnel
// routing), meaning tunnel.dll will install its own blockAll/blockDNS WFP
// filters.
bool ConfigUsesFullTunnelRoutes(const std::string& config);

std::optional<std::string> ValidateKey(const std::string& key,
                                       const std::string& field_name);
std::optional<std::string> ValidateAddress(const std::string& address);
std::optional<std::string> ValidateEndpoint(const std::string& endpoint);

}  // namespace wireguard

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIREGUARD_UTILS_H_
