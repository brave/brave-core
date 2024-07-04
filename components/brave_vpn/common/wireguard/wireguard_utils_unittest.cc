/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/wireguard/wireguard_utils.h"

#include <string>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveVPNWireGuardUtilsUnitTest, ValidateKey) {
  // Invalid.
  // > empty
  EXPECT_FALSE(brave_vpn::wireguard::ValidateKey("", "public_key").has_value());
  // > not base64 encoded
  EXPECT_FALSE(
      brave_vpn::wireguard::ValidateKey("abcdefghi", "public_key").has_value());
  // > not 32 bytes
  EXPECT_FALSE(brave_vpn::wireguard::ValidateKey(
                   base::Base64Encode("abcdefghi"), "public_key")
                   .has_value());
  // > has CR/LF in encoded value
  EXPECT_FALSE(brave_vpn::wireguard::ValidateKey(R"(MsdIM8m7Ee13QbjFe3
fbFtShscNPxYrqQZHvXFnAago=)",
                                                 "public_key")
                   .has_value());

  // Valid.
  EXPECT_TRUE(
      brave_vpn::wireguard::ValidateKey(
          base::Base64Encode("01234567890123456789012345678901"), "public_key")
          .has_value());

  EXPECT_TRUE(brave_vpn::wireguard::ValidateKey(
                  "MsdIM8m7Ee13QbjFe3fbFtShscNPxYrqQZHvXFnAago=", "public_key")
                  .has_value());
  EXPECT_TRUE(brave_vpn::wireguard::ValidateKey(
                  "0h6uFUScpGPOPZgPlEJ1zwcEs+2/CFHYtbLPcoBQYB0=", "public_key")
                  .has_value());
  EXPECT_TRUE(brave_vpn::wireguard::ValidateKey(
                  "l/v3PVoEX618na0q3dwQZigne1xtRPKGqkoDa02a0ac=", "public_key")
                  .has_value());
}

TEST(BraveVPNWireGuardUtilsUnitTest, ValidateAddress) {
  // Invalid.
  EXPECT_FALSE(brave_vpn::wireguard::ValidateAddress("").has_value());
  EXPECT_FALSE(brave_vpn::wireguard::ValidateAddress("a.b.c.d").has_value());
  // IPv6 not allowed.
  EXPECT_FALSE(brave_vpn::wireguard::ValidateAddress("fe80::1ff:fe23:4567:890a")
                   .has_value());
  EXPECT_FALSE(brave_vpn::wireguard::ValidateAddress("1.1.1.1.1").has_value());
  EXPECT_FALSE(brave_vpn::wireguard::ValidateAddress("300.1.1.1").has_value());
  // Spaces are not stripped out.
  // Removed call to base::TrimWhitespaceASCII.
  EXPECT_FALSE(
      brave_vpn::wireguard::ValidateAddress("  192.168.1.1   ").has_value());

  // Valid.
  EXPECT_TRUE(
      brave_vpn::wireguard::ValidateAddress("10.146.91.135").has_value());

  auto response = brave_vpn::wireguard::ValidateAddress("192.168.1.1");
  EXPECT_TRUE(response.has_value());
  // Verify parsing worked.
  EXPECT_EQ(response.value(), "192.168.1.1");
}

TEST(BraveVPNWireGuardUtilsUnitTest, ValidateEndpoint) {
  // Invalid.
  EXPECT_FALSE(brave_vpn::wireguard::ValidateEndpoint("").has_value());
  EXPECT_FALSE(
      brave_vpn::wireguard::ValidateEndpoint("192.168.1.1").has_value());
  EXPECT_FALSE(brave_vpn::wireguard::ValidateEndpoint(
                   "toronto-ipsec-8.not-guardianapp.com")
                   .has_value());
  EXPECT_FALSE(brave_vpn::wireguard::ValidateEndpoint(R"(france-ipsec-1
.sudosecuritygroup.com)")
                   .has_value());
  EXPECT_FALSE(brave_vpn::wireguard::ValidateEndpoint(
                   "france-ipsec-1 .sudosecuritygroup.com")
                   .has_value());
  // Unicode will fail because of base::WideToUTF8.
  // Hostnames realistically should be punycode encoded.
  EXPECT_FALSE(
      brave_vpn::wireguard::ValidateEndpoint("汉字.sudosecuritygroup.com")
          .has_value());

  // Valid.
  EXPECT_TRUE(
      brave_vpn::wireguard::ValidateEndpoint("toronto-ipsec-8.guardianapp.com")
          .has_value());
  EXPECT_TRUE(brave_vpn::wireguard::ValidateEndpoint("a.b.guardianapp.com")
                  .has_value());
  EXPECT_TRUE(brave_vpn::wireguard::ValidateEndpoint(
                  "france-ipsec-1.sudosecuritygroup.com")
                  .has_value());
}
