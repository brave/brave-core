/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/wireguard/wireguard_utils.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidEndpoint_NULL) {
  const wchar_t* null_config = NULL;
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(null_config));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_ValidConfig) {
  const wchar_t* valid_config =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
  )";
  EXPECT_TRUE(brave_vpn::wireguard::IsValidConfig(valid_config));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_ValidConfigOldHostname) {
  const wchar_t* valid_config_old_hostname =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = france-ipsec-1.sudosecuritygroup.com:51821
  )";
  EXPECT_TRUE(brave_vpn::wireguard::IsValidConfig(valid_config_old_hostname));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidCharHash) {
  const wchar_t* invalid_char_config1 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    #AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_char_config1));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidCharLowAscii) {
  const wchar_t* invalid_char_config2 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_char_config2));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidCharHighAscii) {
  const wchar_t* invalid_char_config3 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    €‚
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_char_config3));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidHost) {
  const wchar_t* invalid_endpoint_config1 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.not-guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_endpoint_config1));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidPort) {
  const wchar_t* invalid_endpoint_config2 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:1111
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_endpoint_config2));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_MultipleEndpoints) {
  const wchar_t* invalid_endpoint_config3 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_endpoint_config3));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidEndpointsWhitespace) {
  const wchar_t* invalid_endpoint_config4 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    Endpoint=10.0.1.1:51821
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_endpoint_config4));
}

TEST(BraveVPNWireGuardUtilsUnitTest,
     IsValidConfig_InvalidMultipleEndpointsWhitespace) {
  const wchar_t* invalid_endpoint_config5 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
    Endpoint=10.0.1.1:51821
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_endpoint_config5));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidEndpointCasing) {
  const wchar_t* invalid_endpoint_config6 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
    endpoint = 10.0.1.1:51821
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_endpoint_config6));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidEndpointFormat) {
  const wchar_t* invalid_endpoint_config7 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 10.8.0.2/24
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = [2001:db8::1]:51820
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_endpoint_config7));
}

TEST(BraveVPNWireGuardUtilsUnitTest,
     IsValidConfig_InvalidEndpointSubdomainLength) {
  const wchar_t* invalid_endpoint_config7 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 10.8.0.2/24
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = 1234567890123456789012345678901234567890123456789012345678901234.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_endpoint_config7));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_MultipleEndpointsOneLine) {
  const wchar_t* invalid_endpoint_config8 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 10.8.0.2/24
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821 toronto-ipsec-9.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_endpoint_config8));
}

TEST(BraveVPNWireGuardUtilsUnitTest,
     IsValidConfig_MultipleEndpointsOneLineComma) {
  const wchar_t* invalid_endpoint_config9 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 10.8.0.2/24
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821,toronto-ipsec-9.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_endpoint_config9));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidDNS) {
  const wchar_t* invalid_dns_config1 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 8.8.8.8
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_dns_config1));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidMultipleDNSEntries) {
  const wchar_t* invalid_dns_config2 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    DNS = 1.1.1.1
    Address = 192.168.1.1
    DNS = 1.1.1.1
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_dns_config2));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidDNSWhitespace) {
  const wchar_t* invalid_dns_config3 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS=8.8.8.8
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_dns_config3));
}

TEST(BraveVPNWireGuardUtilsUnitTest,
     IsValidConfig_InvalidMultipleDNSWhitespace) {
  const wchar_t* invalid_dns_config4 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    DNS=8.8.8.8
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_dns_config4));
}

TEST(BraveVPNWireGuardUtilsUnitTest, IsValidConfig_InvalidDNSCasing) {
  const wchar_t* invalid_dns_config3 =
      LR"(
    [Interface]
    PrivateKey = abcdefghi
    Address = 192.168.1.1
    DNS = 1.1.1.1
    dns = 8.8.8.8
    [Peer]
    PublicKey = defghijkl
    AllowedIPs = 0.0.0.0/0, ::/0
    Endpoint = toronto-ipsec-8.guardianapp.com:51821
  )";
  EXPECT_FALSE(brave_vpn::wireguard::IsValidConfig(invalid_dns_config3));
}
