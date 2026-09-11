/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/wireguard/wireguard_utils.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/ip_address.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::wireguard {
extern std::string EncodeBase64(base::span<const uint8_t> in);
}

namespace {

constexpr char kTestPrivateKey[] =
    "MsdIM8m7Ee13QbjFe3fbFtShscNPxYrqQZHvXFnAago=";
constexpr char kTestPublicKey[] =
    "0h6uFUScpGPOPZgPlEJ1zwcEs+2/CFHYtbLPcoBQYB0=";
constexpr char kTestHostname[] = "toronto-ipsec-8.guardianapp.com";
constexpr char kTestAddress[] = "10.146.91.135";

std::string CreateTestConfig() {
  auto config = brave_vpn::wireguard::CreateWireguardConfig(
      kTestPrivateKey, kTestPublicKey, kTestHostname, kTestAddress);
  CHECK(config.has_value());
  return *config;
}

std::vector<std::string> GetAllowedIPs(const std::string& config) {
  constexpr std::string_view kPrefix = "AllowedIPs = ";
  auto start = config.find(kPrefix);
  if (start == std::string::npos) {
    return {};
  }
  std::string_view value(config);
  value.remove_prefix(start + kPrefix.size());
  value = value.substr(0, value.find('\n'));
  return base::SplitString(value, ",", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

// True when `address` is covered by one of the peer's AllowedIPs, meaning it is
// routed into the tunnel instead of staying on the local network.
bool IsRoutedIntoTunnel(const std::vector<std::string>& allowed_ips,
                        const std::string& address) {
  auto parsed = net::IPAddress::FromIPLiteral(address);
  EXPECT_TRUE(parsed.has_value()) << address;
  if (!parsed.has_value()) {
    return false;
  }
  for (const auto& allowed_ip : allowed_ips) {
    net::IPAddress prefix;
    size_t prefix_length = 0;
    EXPECT_TRUE(net::ParseCIDRBlock(allowed_ip, &prefix, &prefix_length))
        << allowed_ip;
    // IPAddressMatchesPrefix() maps IPv4 into IPv4-mapped IPv6 when the
    // families differ, which would let `::/1` match every IPv4 address.
    if (prefix.IsIPv4() != parsed->IsIPv4()) {
      continue;
    }
    if (net::IPAddressMatchesPrefix(parsed.value(), prefix, prefix_length)) {
      return true;
    }
  }
  return false;
}

}  // namespace

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
  // Loopback addresses should be rejected.
  EXPECT_FALSE(brave_vpn::wireguard::ValidateAddress("127.0.0.1").has_value());
  EXPECT_FALSE(
      brave_vpn::wireguard::ValidateAddress("127.255.255.255").has_value());
  // Link-local addresses should be rejected.
  EXPECT_FALSE(
      brave_vpn::wireguard::ValidateAddress("169.254.0.1").has_value());
  EXPECT_FALSE(
      brave_vpn::wireguard::ValidateAddress("169.254.255.255").has_value());
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

// https://github.com/brave/brave-browser/issues/50569
TEST(BraveVPNWireGuardUtilsUnitTest, EncodeBase64) {
  std::vector<unsigned char> key_data(32, 0);
  std::string dangling_key = brave_vpn::wireguard::EncodeBase64(key_data);

  std::vector<uint8_t> overwrite_buffer(45, 1);
  overwrite_buffer.back() = '\0';

  EXPECT_EQ(dangling_key, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");
  EXPECT_EQ(dangling_key.length(), 44u);
}

TEST(BraveVPNWireGuardUtilsUnitTest, CreateWireguardConfig) {
  // Every field is required.
  EXPECT_FALSE(brave_vpn::wireguard::CreateWireguardConfig(
                   "", kTestPublicKey, kTestHostname, kTestAddress)
                   .has_value());
  EXPECT_FALSE(brave_vpn::wireguard::CreateWireguardConfig(
                   kTestPrivateKey, "", kTestHostname, kTestAddress)
                   .has_value());
  EXPECT_FALSE(brave_vpn::wireguard::CreateWireguardConfig(
                   kTestPrivateKey, kTestPublicKey, "", kTestAddress)
                   .has_value());
  EXPECT_FALSE(brave_vpn::wireguard::CreateWireguardConfig(
                   kTestPrivateKey, kTestPublicKey, kTestHostname, "")
                   .has_value());

  auto config = CreateTestConfig();
  EXPECT_THAT(config, testing::HasSubstr("PrivateKey = " +
                                         std::string(kTestPrivateKey)));
  EXPECT_THAT(config,
              testing::HasSubstr("PublicKey = " + std::string(kTestPublicKey)));
  EXPECT_THAT(config,
              testing::HasSubstr("Address = " + std::string(kTestAddress)));
  EXPECT_THAT(config,
              testing::HasSubstr("Endpoint = " + std::string(kTestHostname) +
                                 ":51821"));
  EXPECT_THAT(config, testing::HasSubstr("DNS = 1.1.1.1"));
  // No placeholder left unsubstituted.
  EXPECT_EQ(config.find('{'), std::string::npos);
}

TEST(BraveVPNWireGuardUtilsUnitTest, WireguardConfigHasNoDefaultRoute) {
  auto allowed_ips = GetAllowedIPs(CreateTestConfig());
  EXPECT_THAT(allowed_ips, testing::ElementsAre("0.0.0.0/1", "128.0.0.0/1",
                                                "::/1", "8000::/1"));

  // A default route makes tunnel.dll install its own blockAll/blockDNS WFP
  // filters, which block the local network. We install our own filter set
  // instead, so no prefix here may be a /0.
  for (const auto& allowed_ip : allowed_ips) {
    net::IPAddress prefix;
    size_t prefix_length = 0;
    ASSERT_TRUE(net::ParseCIDRBlock(allowed_ip, &prefix, &prefix_length))
        << allowed_ip;
    EXPECT_NE(prefix_length, 0u) << allowed_ip;
  }

  // Splitting the default route must not shrink coverage: everything still
  // routes into the tunnel, in both halves of both address families.
  constexpr const char* kTunneledAddresses[] = {
      "0.0.0.1",   "1.1.1.1",      "8.8.8.8",     "127.255.255.1",
      "128.0.0.1", "192.168.1.5",  "224.0.0.251", "255.255.255.255",
      "::1",       "2606:4700::1", "fd00::1",     "fe80::1",
      "ff02::fb",
  };
  for (const auto* address : kTunneledAddresses) {
    EXPECT_TRUE(IsRoutedIntoTunnel(allowed_ips, address)) << address;
  }
}
