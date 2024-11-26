/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_utils.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;
using testing::IsEmpty;

namespace brave_wallet {

TEST(HDKeyUtilsUnitTest, ParseFullHDPath) {
  // Success cases.
  EXPECT_THAT(*ParseFullHDPath("m"), IsEmpty());

  EXPECT_THAT(*ParseFullHDPath("m/0"), ElementsAre(0));
  EXPECT_THAT(*ParseFullHDPath("m/1"), ElementsAre(1));

  EXPECT_THAT(*ParseFullHDPath("m/0'"), ElementsAre(kHardenedOffset));
  EXPECT_THAT(*ParseFullHDPath("m/2'"), ElementsAre(kHardenedOffset + 2));

  EXPECT_THAT(*ParseFullHDPath("m/0/1/2/3/4"), ElementsAre(0, 1, 2, 3, 4));
  EXPECT_THAT(*ParseFullHDPath("m/2'/3/4'/5"),
              ElementsAre(kHardenedOffset + 2, 3, kHardenedOffset + 4, 5));

  // Index overflows.
  EXPECT_THAT(*ParseFullHDPath("m/2147483647"), ElementsAre(2147483647));
  EXPECT_FALSE(ParseFullHDPath("m/2147483648"));
  EXPECT_THAT(*ParseFullHDPath("m/2147483647'"),
              ElementsAre(kHardenedOffset + 2147483647));
  EXPECT_FALSE(ParseFullHDPath("m/2147483648'"));

  // Incorrect format.
  EXPECT_FALSE(ParseFullHDPath(""));
  EXPECT_FALSE(ParseFullHDPath("a"));
  EXPECT_FALSE(ParseFullHDPath("/0/1/2/3/4"));
  EXPECT_FALSE(ParseFullHDPath("0/1/2/3/4"));
  EXPECT_FALSE(ParseFullHDPath("m/0//1"));
  EXPECT_FALSE(ParseFullHDPath("m/0/1/"));
  EXPECT_FALSE(ParseFullHDPath("m/-1"));
  EXPECT_FALSE(ParseFullHDPath("m/1/a"));
  EXPECT_FALSE(ParseFullHDPath("m/1''"));
  EXPECT_FALSE(ParseFullHDPath("m/1'1"));
}

}  // namespace brave_wallet
