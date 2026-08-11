// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/bnes/bns_security.h"

#include "net/base/ip_address.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace bnes {

TEST(BnsSecurityTest, AcceptsOnlyUnambiguousBnesNavigation) {
  EXPECT_TRUE(IsAllowedNavigationUrl(GURL("bnes://bear.bnes/index.html")));
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("https://bear.bnes/")));
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("bnes://bnes/")));
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("bnes://127.0.0.1/")));
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("bnes://user@bear.bnes/")));
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("bnes://bear.bnes:443/")));
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("bnes://-bear.bnes/")));
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("bnes://bear-.bnes/")));
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("bnes://bear..bnes/")));
  EXPECT_TRUE(IsAllowedNavigationUrl(GURL("bnes://bear.bnes/?page=about")));
}

TEST(BnsSecurityTest, PinsGatewayToCanonicalHttpsOrigin) {
  constexpr char kGateway[] = "ipfs.bearnetwork.net";
  EXPECT_TRUE(IsAllowedGatewayUrl(
      GURL("https://ipfs.bearnetwork.net/ipfs/bafybeigdyrzt"), kGateway));
  EXPECT_FALSE(IsAllowedGatewayUrl(
      GURL("http://ipfs.bearnetwork.net/ipfs/bafybeigdyrzt"), kGateway));
  EXPECT_FALSE(IsAllowedGatewayUrl(
      GURL("https://ipfs.bearnetwork.net.evil.test/ipfs/bafybeigdyrzt"),
      kGateway));
  EXPECT_FALSE(IsAllowedGatewayUrl(
      GURL("https://ipfs.bearnetwork.net@evil.test/ipfs/bafybeigdyrzt"),
      kGateway));
}

TEST(BnsSecurityTest, RejectsReservedAddressRangesAfterDnsResolution) {
  EXPECT_FALSE(IsPublicNetworkAddress(net::IPAddress::IPv4Localhost()));
  auto private_address = net::IPAddress::FromIPLiteral("192.168.1.1");
  auto private_10_address = net::IPAddress::FromIPLiteral("10.0.0.1");
  auto public_address = net::IPAddress::FromIPLiteral("1.1.1.1");
  ASSERT_TRUE(private_address);
  ASSERT_TRUE(private_10_address);
  ASSERT_TRUE(public_address);
  EXPECT_FALSE(IsPublicNetworkAddress(*private_address));
  EXPECT_FALSE(IsPublicNetworkAddress(*private_10_address));
  EXPECT_TRUE(IsPublicNetworkAddress(*public_address));
}

TEST(BnsSecurityTest, RejectsCidUrlInjection) {
  EXPECT_TRUE(IsValidCid("bafybeigdyrzt"));
  EXPECT_TRUE(IsValidCid("QmYwAPJzv5CZsnAzt8auVZRnGxQK1dH5vzyMGrJMAbXKMF"));
  EXPECT_FALSE(IsValidCid("bafy/beigdyrzt"));
  EXPECT_FALSE(IsValidCid("bafybeigdyrzt?redirect=local"));
  EXPECT_FALSE(IsValidCid("Bafybeigdyrzt"));
  EXPECT_FALSE(IsValidCid("bafybeigdyrz0"));
  EXPECT_FALSE(IsValidCid("QmYwAPJzv5CZsnAzt8auVZRnGxQK1dH5vzyMGrJMAbXKM0"));
  EXPECT_FALSE(IsValidCid("abc"));
}

TEST(BnsSecurityTest, RejectsIPv6ReservedAndLinkLocalAddresses) {
  auto loopback_v6 = net::IPAddress::FromIPLiteral("::1");
  auto link_local_v6 = net::IPAddress::FromIPLiteral("fe80::1");
  auto unique_local_v6 = net::IPAddress::FromIPLiteral("fc00::1");
  auto public_v6 = net::IPAddress::FromIPLiteral("2606:4700:4700::1111");
  ASSERT_TRUE(loopback_v6);
  ASSERT_TRUE(link_local_v6);
  ASSERT_TRUE(unique_local_v6);
  ASSERT_TRUE(public_v6);
  EXPECT_FALSE(IsPublicNetworkAddress(*loopback_v6));
  EXPECT_FALSE(IsPublicNetworkAddress(*link_local_v6));
  EXPECT_FALSE(IsPublicNetworkAddress(*unique_local_v6));
  EXPECT_TRUE(IsPublicNetworkAddress(*public_v6));
}

TEST(BnsSecurityTest, AllowsQueryAndFragmentInNavigation) {
  EXPECT_TRUE(IsAllowedNavigationUrl(GURL("bnes://bear.bnes?foo=bar")));
  EXPECT_TRUE(IsAllowedNavigationUrl(GURL("bnes://bear.bnes#section")));
  EXPECT_TRUE(IsAllowedNavigationUrl(GURL("bnes://bear.bnes/?foo=bar")));
  EXPECT_TRUE(IsAllowedNavigationUrl(GURL("bnes://bear.bnes/#section")));
}

TEST(BnsSecurityTest, RejectsEmptyAndApexBnesHosts) {
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("bnes://")));
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("bnes://.bnes")));
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("bnes://..bnes")));
  EXPECT_FALSE(IsAllowedNavigationUrl(GURL("bnes://bear.")));
}

TEST(BnsSecurityTest, RejectsGatewayWithNonCanonicalPort) {
  constexpr char kGateway[] = "ipfs.bearnetwork.net";
  EXPECT_FALSE(IsAllowedGatewayUrl(
      GURL("https://ipfs.bearnetwork.net:8443/ipfs/bafybeigdyrzt"), kGateway));
}

TEST(BnsSecurityTest, ValidatesBnesHostLabels) {
  EXPECT_TRUE(IsValidBnesHost("bear.bnes"));
  EXPECT_TRUE(IsValidBnesHost("my-site.bnes"));
  EXPECT_TRUE(IsValidBnesHost("a.bnes"));
  EXPECT_TRUE(IsValidBnesHost("BEAR.BNES"));
  EXPECT_TRUE(IsValidBnesHost("My-Site.Bnes"));
  EXPECT_TRUE(IsValidBnesHost(
      std::string(63, 'a') + ".bnes"));
  EXPECT_FALSE(IsValidBnesHost(
      std::string(64, 'a') + ".bnes"));
  EXPECT_FALSE(IsValidBnesHost(""));
  EXPECT_FALSE(IsValidBnesHost(".bnes"));
  EXPECT_FALSE(IsValidBnesHost("..bnes"));
  EXPECT_FALSE(IsValidBnesHost("bear."));
  EXPECT_FALSE(IsValidBnesHost("-bear.bnes"));
  EXPECT_FALSE(IsValidBnesHost("bear-.bnes"));
  EXPECT_FALSE(IsValidBnesHost("bear..bnes"));
  EXPECT_FALSE(IsValidBnesHost("bear.bnes."));
  EXPECT_FALSE(IsValidBnesHost("bear.bnes.."));
  EXPECT_FALSE(IsValidBnesHost("bear..bnes"));
  EXPECT_FALSE(IsValidBnesHost("b"));
  EXPECT_FALSE(IsValidBnesHost("b.b"));
}

}  // namespace bnes
