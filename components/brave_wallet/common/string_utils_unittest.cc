/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/string_utils.h"

#include <limits>

#include "base/logging.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Eq;
using ::testing::Optional;

namespace brave_wallet {

TEST(StringUtilsUnitTest, IsValidBase10String) {
  ASSERT_TRUE(IsValidBase10String("0"));
  ASSERT_TRUE(IsValidBase10String("1"));
  ASSERT_TRUE(IsValidBase10String("-1"));
  ASSERT_TRUE(IsValidBase10String("1234567891011121314"));
  ASSERT_TRUE(IsValidBase10String("-1234567891011121314"));
  // Can have 0's before
  ASSERT_TRUE(IsValidBase10String("0123"));
  ASSERT_TRUE(IsValidBase10String("-0123"));
  ASSERT_FALSE(IsValidBase10String("0x0"));
  ASSERT_FALSE(IsValidBase10String("123A"));
  ASSERT_FALSE(IsValidBase10String(""));
  ASSERT_FALSE(IsValidBase10String("hello world"));
  ASSERT_FALSE(IsValidBase10String("12$$"));
}

TEST(StringUtilsUnitTest, Base10ValueToUint256) {
  EXPECT_THAT(Base10ValueToUint256("0"), Optional(uint256_t{0u}));
  EXPECT_THAT(Base10ValueToUint256("1"), Optional(uint256_t{1}));
  EXPECT_THAT(Base10ValueToUint256("12345678910"),
              Optional(uint256_t{12345678910}));

  // Max uint256 value can be represented
  EXPECT_THAT(Base10ValueToUint256("1157920892373161954235709850086879078532699"
                                   "84665640564039457584007913129639935"),
              Optional(std::numeric_limits<uint256_t>::max()));

  // Should return false when out of bounds
  EXPECT_FALSE(Base10ValueToUint256("115792089237316195423570985008687907853269"
                                    "984665640564039457584007913129639936")
                   .has_value());
  EXPECT_FALSE(Base10ValueToUint256("0xB").has_value());

  // Check padded values too
  EXPECT_THAT(Base10ValueToUint256("0000000000000000000000010"),
              Optional(uint256_t{10}));
}

TEST(StringUtilsUnitTest, Base10ValueToInt256) {
  EXPECT_THAT(Base10ValueToInt256("0"), Optional(int256_t{0}));
  EXPECT_THAT(Base10ValueToInt256("1"), Optional(int256_t{1}));
  EXPECT_THAT(Base10ValueToInt256("-1"), Optional(int256_t{-1}));
  EXPECT_THAT(Base10ValueToInt256("12345678910"),
              Optional(int256_t{12345678910}));
  EXPECT_THAT(Base10ValueToInt256("-12345678910"),
              Optional(int256_t{-12345678910}));

  // Max int256 value can be represented
  EXPECT_THAT(
      Base10ValueToInt256("5789604461865809771178549250434395392663499233282028"
                          "2019728792003956564819967"),
      Optional(kMax256BitInt));

  // Min int256 value can be represented
  EXPECT_THAT(Base10ValueToInt256(
                  "-5789604461865809771178549250434395392663499233282028"
                  "2019728792003956564819968"),
              Optional(kMin256BitInt));

  // Should return false when out of bounds
  EXPECT_FALSE(
      Base10ValueToInt256("5789604461865809771178549250434395392663499233282028"
                          "2019728792003956564819968")
          .has_value());

  EXPECT_FALSE(Base10ValueToInt256(
                   "-5789604461865809771178549250434395392663499233282028"
                   "2019728792003956564819969")
                   .has_value());

  EXPECT_FALSE(Base10ValueToInt256("0xB").has_value());

  // Check padded values too
  EXPECT_THAT(Base10ValueToInt256("0000000000000000000000010"),
              Optional(int256_t{10}));
}

TEST(StringUtilsUnitTest, Uint256ValueToBase10) {
  uint256_t input = 0;
  EXPECT_EQ(Uint256ValueToBase10(input), "0");

  input = 1;
  EXPECT_EQ(Uint256ValueToBase10(input), "1");

  input = 10;
  EXPECT_EQ(Uint256ValueToBase10(input), "10");

  input = 12345678910;
  EXPECT_EQ(Uint256ValueToBase10(input), "12345678910");

  input = std::numeric_limits<uint256_t>::max();
  EXPECT_EQ(Uint256ValueToBase10(input),
            "115792089237316195423570985008687907853269984665640564039457584007"
            "913129639935");

  input = uint256_t(1) << 255;
  EXPECT_EQ(Uint256ValueToBase10(input),
            "578960446186580977117854925043439539266349923328202820197287920039"
            "56564819968");

  input = uint256_t(1) << 128;
  EXPECT_EQ(Uint256ValueToBase10(input),
            "340282366920938463463374607431768211456");
}

}  // namespace brave_wallet
