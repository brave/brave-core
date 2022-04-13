/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {

TEST(EthereumPermissionUtilsUnitTest, GetConcatOriginFromWalletAddresses) {
  std::vector<std::string> addrs = {
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"};
  url::Origin origin = url::Origin::Create(GURL("https://test.com"));
  url::Origin out_origin;
  url::Origin expected_out_origin = url::Origin::Create(
      GURL("https://"
           "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
           "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}"));
  EXPECT_TRUE(GetConcatOriginFromWalletAddresses(origin, addrs, &out_origin));
  EXPECT_EQ(out_origin, expected_out_origin);

  EXPECT_FALSE(
      GetConcatOriginFromWalletAddresses(url::Origin(), addrs, &out_origin));
  EXPECT_FALSE(GetConcatOriginFromWalletAddresses(
      origin, std::vector<std::string>(), &out_origin));

  // Origin with port case:
  expected_out_origin = url::Origin::Create(
      GURL("https://"
           "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
           "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}:123"));

  EXPECT_TRUE(GetConcatOriginFromWalletAddresses(
      url::Origin::Create(GURL("https://test.com:123")), addrs, &out_origin));
  EXPECT_EQ(out_origin, expected_out_origin);
}

TEST(EthereumPermissionUtilsUnitTest, ParseRequestingOriginFromSubRequest) {
  // Invalid requesting_origin format:
  EXPECT_FALSE(ParseRequestingOriginFromSubRequest(
      url::Origin::Create(GURL("https://test.com0x123")), nullptr, nullptr));
  EXPECT_FALSE(ParseRequestingOriginFromSubRequest(
      url::Origin::Create(GURL("https://test.com0x123/path")), nullptr,
      nullptr));
  EXPECT_FALSE(ParseRequestingOriginFromSubRequest(
      url::Origin::Create(GURL("")), nullptr, nullptr));

  url::Origin requesting_origin;
  std::string account;
  EXPECT_TRUE(ParseRequestingOriginFromSubRequest(
      url::Origin::Create(
          GURL("https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A")),
      &requesting_origin, &account));
  EXPECT_EQ(requesting_origin, url::Origin::Create(GURL("https://test.com")));
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(
      account, "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A"));
  EXPECT_TRUE(ParseRequestingOriginFromSubRequest(
      url::Origin::Create(GURL(
          "https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A:123")),
      &requesting_origin, &account));
  EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test.com:123"));
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(
      account, "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A"));
}

TEST(EthereumPermissionUtilsUnitTest, ParseRequestingOrigin) {
  // Invalid requesting_origin format:
  EXPECT_FALSE(ParseRequestingOrigin(
      url::Origin::Create(GURL("https://test.com0x123")), nullptr, nullptr));
  EXPECT_FALSE(ParseRequestingOrigin(
      url::Origin::Create(GURL("https://test.com0x123/path")), nullptr,
      nullptr));
  EXPECT_FALSE(
      ParseRequestingOrigin(url::Origin::Create(GURL("")), nullptr, nullptr));

  std::queue<std::string> address_queue;

  // Origin without port:
  url::Origin requesting_origin;
  EXPECT_TRUE(ParseRequestingOrigin(
      url::Origin::Create(GURL(
          "https://test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A}")),
      &requesting_origin, &address_queue));
  EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test.com"));
  EXPECT_EQ(address_queue.size(), 1u);
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(
      address_queue.front(), "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A"));

  EXPECT_TRUE(ParseRequestingOrigin(
      url::Origin::Create(
          GURL("https://"
               "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
               "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}")),
      &requesting_origin, nullptr));
  EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test.com"));

  // Origin with port:
  EXPECT_TRUE(ParseRequestingOrigin(
      url::Origin::Create(GURL(
          "https://"
          "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A}:123")),
      &requesting_origin, nullptr));
  EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test.com:123"));

  EXPECT_FALSE(ParseRequestingOrigin(
      url::Origin::Create(
          GURL("https://"
               "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
               "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}:123")),
      &requesting_origin, &address_queue))
      << "Non-empty address_queue param should return false";
  address_queue = std::queue<std::string>();
  EXPECT_TRUE(ParseRequestingOrigin(
      url::Origin::Create(
          GURL("https://"
               "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
               "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}:123")),
      &requesting_origin, &address_queue));
  EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test.com:123"));
  EXPECT_EQ(address_queue.size(), 2u);
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(
      address_queue.front(), "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A"));
  address_queue.pop();
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(
      address_queue.front(), "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"));
}

TEST(EthereumPermissionUtilsUnitTest, GetSubRequestOrigin) {
  url::Origin new_origin;
  url::Origin old_origin = url::Origin::Create(GURL("https://test.com"));
  url::Origin old_origin_with_port =
      url::Origin::Create(GURL("https://test.com:123"));
  std::string account = "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B";
  url::Origin expected_new_origin = url::Origin::Create(
      GURL("https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"));
  url::Origin expected_new_origin_with_port = url::Origin::Create(
      GURL("https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B:123"));

  EXPECT_FALSE(GetSubRequestOrigin(url::Origin(), account, &new_origin));
  EXPECT_FALSE(GetSubRequestOrigin(old_origin, "", &new_origin));
  EXPECT_FALSE(GetSubRequestOrigin(old_origin, account, nullptr));

  EXPECT_TRUE(GetSubRequestOrigin(old_origin, account, &new_origin));
  EXPECT_EQ(new_origin, expected_new_origin);
  EXPECT_TRUE(GetSubRequestOrigin(old_origin_with_port, account, &new_origin));
  EXPECT_EQ(new_origin, expected_new_origin_with_port);
}

TEST(EthereumPermissionUtilsUnitTest, GetConnectWithSiteWebUIURL) {
  GURL base_url("chrome://wallet-panel.top-chrome/");
  std::vector<std::string> addrs = {
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"};
  url::Origin origin = url::Origin::Create(GURL("https://a.test.com:123"));
  GURL url_out = GetConnectWithSiteWebUIURL(base_url, addrs, origin);
  EXPECT_EQ(url_out.spec(),
            "chrome://wallet-panel.top-chrome/"
            "?addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
            "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B&origin-scheme=https&"
            "origin-host=a.test.com&origin-port=123&origin-spec=https://"
            "a.test.com:123&etld-plus-one=test.com#connectWithSite");
}

}  // namespace brave_wallet
