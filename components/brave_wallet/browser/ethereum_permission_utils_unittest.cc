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

namespace brave_wallet {

TEST(EthereumPermissionUtilsUnitTest, GetConcatOriginFromWalletAddresses) {
  std::vector<std::string> addrs = {
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"};
  GURL origin("https://test.com");
  GURL out_origin;
  GURL expected_out_origin(
      "https://"
      "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}");
  EXPECT_TRUE(GetConcatOriginFromWalletAddresses(origin, addrs, &out_origin));
  EXPECT_EQ(out_origin, expected_out_origin);

  EXPECT_FALSE(
      GetConcatOriginFromWalletAddresses(GURL(""), addrs, &out_origin));
  EXPECT_FALSE(GetConcatOriginFromWalletAddresses(
      origin, std::vector<std::string>(), &out_origin));

  // Origin with port case:
  expected_out_origin = GURL(
      "https://"
      "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}:123");

  EXPECT_TRUE(GetConcatOriginFromWalletAddresses(GURL("https://test.com:123"),
                                                 addrs, &out_origin));
  EXPECT_EQ(out_origin, expected_out_origin);
}

TEST(EthereumPermissionUtilsUnitTest, ParseRequestingOrigin) {
  // Invalid requesting_origin format:
  EXPECT_FALSE(ParseRequestingOrigin(GURL("https://test.com0x123"), true,
                                     nullptr, nullptr));
  EXPECT_FALSE(ParseRequestingOrigin(GURL("https://test.com0x123"), false,
                                     nullptr, nullptr));
  EXPECT_FALSE(ParseRequestingOrigin(GURL(""), true, nullptr, nullptr));
  EXPECT_FALSE(ParseRequestingOrigin(GURL(""), false, nullptr, nullptr));

  // Sub-req format:
  std::string requesting_origin;
  std::string account;
  EXPECT_TRUE(ParseRequestingOrigin(
      GURL("https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A"), true,
      &requesting_origin, &account));
  EXPECT_EQ(requesting_origin, "https://test.com");
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(
      account, "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A"));
  EXPECT_TRUE(ParseRequestingOrigin(
      GURL("https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A:123"),
      true, &requesting_origin, &account));
  EXPECT_EQ(requesting_origin, "https://test.com:123");
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(
      account, "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A"));

  // Non-sub-req format:
  requesting_origin = "";
  EXPECT_TRUE(ParseRequestingOrigin(
      GURL("https://test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A}"),
      false, &requesting_origin, nullptr));
  EXPECT_EQ(requesting_origin, "https://test.com");

  EXPECT_TRUE(ParseRequestingOrigin(
      GURL("https://"
           "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
           "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}"),
      false, &requesting_origin, nullptr));
  EXPECT_EQ(requesting_origin, "https://test.com");

  // Non-sub-req format with port:
  EXPECT_TRUE(ParseRequestingOrigin(
      GURL("https://"
           "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A}:123"),
      false, &requesting_origin, nullptr));
  EXPECT_EQ(requesting_origin, "https://test.com:123");

  EXPECT_TRUE(ParseRequestingOrigin(
      GURL("https://"
           "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
           "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}:123"),
      false, &requesting_origin, nullptr));
  EXPECT_EQ(requesting_origin, "https://test.com:123");
}

TEST(EthereumPermissionUtilsUnitTest, GetSubRequestOrigin) {
  GURL new_origin;
  GURL old_origin("https://test.com");
  GURL old_origin_with_port("https://test.com:123");
  std::string account = "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B";
  GURL expected_new_origin =
      GURL("https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B");
  GURL expected_new_origin_with_port =
      GURL("https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B:123");

  EXPECT_FALSE(GetSubRequestOrigin(GURL(""), account, &new_origin));
  EXPECT_FALSE(GetSubRequestOrigin(old_origin, "", &new_origin));
  EXPECT_FALSE(GetSubRequestOrigin(old_origin, account, nullptr));

  EXPECT_TRUE(GetSubRequestOrigin(old_origin, account, &new_origin));
  EXPECT_EQ(new_origin, expected_new_origin);
  EXPECT_TRUE(GetSubRequestOrigin(old_origin_with_port, account, &new_origin));
  EXPECT_EQ(new_origin, expected_new_origin_with_port);
}

}  // namespace brave_wallet
