/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(PolkadotUtils, DestinationAddressParsing) {
  // Account at:
  // https://assethub-westend.subscan.io/account/5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
  // https://polkadot.subscan.io/account/14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3

  EXPECT_EQ(ParsePolkadotAccount(
                "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty", 42)
                .value()
                .ToString(),
            "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty");

  EXPECT_EQ(ParsePolkadotAccount(
                "14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3", 0)
                .value()
                .ToString(),
            "14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3");

  EXPECT_EQ(
      ParsePolkadotAccount(
          R"(0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48)",
          0)
          .value()
          .ToString(),
      "0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48");

  // Address isn't 0x-prefixed
  EXPECT_FALSE(ParsePolkadotAccount(
      "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48", 0));

  // Invalid ss58 prefix.
  EXPECT_FALSE(ParsePolkadotAccount(
      "4FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty", 42));
  EXPECT_FALSE(ParsePolkadotAccount(
      "24E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3", 0));

  // Address is too long.
  EXPECT_FALSE(ParsePolkadotAccount(
      "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty694ty", 42));
  EXPECT_FALSE(ParsePolkadotAccount(
      "0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a481234",
      0));
  EXPECT_FALSE(ParsePolkadotAccount(
      R"(0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a481234)",
      42));

  // Address is too short.
  EXPECT_FALSE(ParsePolkadotAccount(
      "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694t", 42));
  EXPECT_FALSE(ParsePolkadotAccount(
      "0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4", 0));
  EXPECT_FALSE(ParsePolkadotAccount(
      "0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a", 0));
  EXPECT_FALSE(ParsePolkadotAccount("", 0));

  // Random nonsense.
  EXPECT_FALSE(ParsePolkadotAccount("random string full of random words", 0));
}

}  // namespace brave_wallet
