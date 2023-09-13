/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/mac/brave_vpn_wireguard_bridge/utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

namespace {
constexpr char kWireguardConfigTest[] = R"(
  [Interface]
  PrivateKey = {client_private_key}
  Address = {mapped_ipv4_address}
  DNS = {dns_servers}
  [Peer]
  PublicKey = {server_public_key}
  AllowedIPs = 0.0.0.0/0, ::/0
  Endpoint = 127.0.0.1:1111
)";
}  // namespace

TEST(BraveVpnWireguardUtilsUnitTest, GetConfigStringValue) {
  EXPECT_EQ(*GetConfigStringValue("Endpoint", kWireguardConfigTest),
            "127.0.0.1:1111");

  EXPECT_EQ(*GetConfigStringValue("endpoint", kWireguardConfigTest),
            "127.0.0.1:1111");

  EXPECT_FALSE(GetConfigStringValue("NotFound", kWireguardConfigTest));
  EXPECT_FALSE(GetConfigStringValue("Endpoint", std::string()));
}

}  // namespace brave_vpn
