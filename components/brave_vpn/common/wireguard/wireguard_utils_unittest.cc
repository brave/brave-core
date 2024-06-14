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
  std::string output;
  // Invalid.
  // > empty
  EXPECT_FALSE(brave_vpn::wireguard::ValidateKey(L"", &output, "public_key"));
  // > not base64 encoded
  EXPECT_FALSE(
      brave_vpn::wireguard::ValidateKey(L"abcdefghi", &output, "public_key"));
  // > not 32 bytes
  std::wstring key_str = base::UTF8ToWide(base::Base64Encode("abcdefghi"));
  EXPECT_FALSE(brave_vpn::wireguard::ValidateKey(key_str.c_str(), &output,
                                                 "public_key"));
  // > has CR/LF in encoded value
  EXPECT_FALSE(
      brave_vpn::wireguard::ValidateKey(LR"(MsdIM8m7Ee13QbjFe3
fbFtShscNPxYrqQZHvXFnAago=)",
                                        &output, "public_key"));

  // Valid.
  key_str =
      base::UTF8ToWide(base::Base64Encode("01234567890123456789012345678901"));
  EXPECT_TRUE(brave_vpn::wireguard::ValidateKey(key_str.c_str(), &output,
                                                "public_key"));

  EXPECT_TRUE(brave_vpn::wireguard::ValidateKey(
      L"MsdIM8m7Ee13QbjFe3fbFtShscNPxYrqQZHvXFnAago=", &output, "public_key"));
}

TEST(BraveVPNWireGuardUtilsUnitTest, ValidateAddress) {
  std::string output;
  // Invalid.
  EXPECT_FALSE(brave_vpn::wireguard::ValidateAddress(L"", &output));
  EXPECT_FALSE(brave_vpn::wireguard::ValidateAddress(L"a.b.c.d", &output));
  // IPv6 not allowed.
  EXPECT_FALSE(brave_vpn::wireguard::ValidateAddress(
      L"fe80::1ff:fe23:4567:890a", &output));

  // Valid.
  EXPECT_TRUE(brave_vpn::wireguard::ValidateAddress(L"192.168.1.1", &output));
  // Spaces are stripped out.
  EXPECT_TRUE(
      brave_vpn::wireguard::ValidateAddress(L"  192.168.1.1   ", &output));
  // Verify parsing worked.
  EXPECT_EQ(output, "192.168.1.1");
}

TEST(BraveVPNWireGuardUtilsUnitTest, ValidateEndpoint) {
  std::string output;
  EXPECT_FALSE(brave_vpn::wireguard::ValidateEndpoint(L"", &output));
  EXPECT_FALSE(brave_vpn::wireguard::ValidateEndpoint(L"192.168.1.1", &output));
  EXPECT_FALSE(brave_vpn::wireguard::ValidateEndpoint(
      L"toronto-ipsec-8.not-guardianapp.com", &output));
  EXPECT_FALSE(
      brave_vpn::wireguard::ValidateEndpoint(LR"(france-ipsec-1
.sudosecuritygroup.com)",
                                             &output));
  EXPECT_FALSE(brave_vpn::wireguard::ValidateEndpoint(
      L"france-ipsec-1 .sudosecuritygroup.com", &output));

  EXPECT_TRUE(brave_vpn::wireguard::ValidateEndpoint(
      L"toronto-ipsec-8.guardianapp.com", &output));
  EXPECT_TRUE(brave_vpn::wireguard::ValidateEndpoint(
      L"france-ipsec-1.sudosecuritygroup.com", &output));
}
