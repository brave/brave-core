/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"

#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(PolkadotUtils, DestinationAddressParsing) {
  // Account at:
  // https://assethub-westend.subscan.io/account/5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
  // https://polkadot.subscan.io/account/14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3

  EXPECT_EQ(base::HexEncodeLower(
                ParsePolkadotAccount(
                    "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty")
                    .value()),
            "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48");

  EXPECT_EQ(base::HexEncodeLower(
                ParsePolkadotAccount(
                    "14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3")
                    .value()),
            "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48");

  EXPECT_EQ(base::HexEncodeLower(
                ParsePolkadotAccount("8eaf04151687736326c9fea17e25fc5287613693c"
                                     "912909cb226aa4794f26a48")
                    .value()),
            "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48");

  EXPECT_EQ(base::HexEncodeLower(
                ParsePolkadotAccount("0x8eaf04151687736326c9fea17e25fc528761369"
                                     "3c912909cb226aa4794f26a48")
                    .value()),
            "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48");

  // Invalid ss58 prefix.
  EXPECT_FALSE(
      ParsePolkadotAccount("4FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty"));
  EXPECT_FALSE(
      ParsePolkadotAccount("24E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3"));

  // Address is too long.
  EXPECT_FALSE(ParsePolkadotAccount(
      "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty694ty"));
  EXPECT_FALSE(ParsePolkadotAccount(
      "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a481234"));
  EXPECT_FALSE(
      ParsePolkadotAccount("0x8eaf04151687736326c9fea17e25fc5287613693c912909cb"
                           "226aa4794f26a481234"));

  // Address is too short.
  EXPECT_FALSE(
      ParsePolkadotAccount("5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694t"));
  EXPECT_FALSE(ParsePolkadotAccount(
      "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4"));
  EXPECT_FALSE(ParsePolkadotAccount(
      "0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a"));
  EXPECT_FALSE(ParsePolkadotAccount(""));

  // Random nonsense.
  EXPECT_FALSE(ParsePolkadotAccount("random string full of random words"));
}

}  // namespace brave_wallet
