/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIREGUARD_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIREGUARD_UTILS_H_

#include <optional>
#include <string>
#include <tuple>

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
    const std::string& mapped_ipv4_address);

WireguardKeyPair GenerateNewX25519Keypair();

std::optional<std::string> ValidateKey(const std::string& key,
                                       const std::string& field_name);
std::optional<std::string> ValidateAddress(const std::string& address);
std::optional<std::string> ValidateEndpoint(const std::string& endpoint);

}  // namespace wireguard

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_WIREGUARD_UTILS_H_
