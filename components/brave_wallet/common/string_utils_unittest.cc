/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/string_utils.h"

#include <limits>

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

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
  uint256_t out;
  ASSERT_TRUE(Base10ValueToUint256("0", &out));
  ASSERT_EQ(out, (uint256_t)0);
  ASSERT_TRUE(Base10ValueToUint256("1", &out));
  ASSERT_EQ(out, (uint256_t)1);
  ASSERT_TRUE(Base10ValueToUint256("12345678910", &out));
  ASSERT_EQ(out, (uint256_t)12345678910);

  // Max uint256 value can be represented
  uint256_t expected_val = std::numeric_limits<uint256_t>::max();
  ASSERT_TRUE(
      Base10ValueToUint256("115792089237316195423570985008687907853269984665640"
                           "564039457584007913129639935",
                           &out));
  ASSERT_TRUE(out == (uint256_t)expected_val);

  // Should return false when out of bounds
  ASSERT_FALSE(
      Base10ValueToUint256("115792089237316195423570985008687907853269984665640"
                           "564039457584007913129639936",
                           &out));
  ASSERT_FALSE(Base10ValueToUint256("0xB", &out));

  // Check padded values too
  ASSERT_TRUE(Base10ValueToUint256("0000000000000000000000010", &out));
  ASSERT_EQ(out, (uint256_t)10);
}

TEST(StringUtilsUnitTest, Base10ValueToInt256) {
  int256_t out;
  ASSERT_TRUE(Base10ValueToInt256("0", &out));
  EXPECT_EQ(out, int256_t(0));
  ASSERT_TRUE(Base10ValueToInt256("1", &out));
  EXPECT_EQ(out, int256_t(1));
  ASSERT_TRUE(Base10ValueToInt256("-1", &out));
  EXPECT_EQ(out, int256_t(-1));
  ASSERT_TRUE(Base10ValueToInt256("12345678910", &out));
  EXPECT_EQ(out, int256_t(12345678910));
  ASSERT_TRUE(Base10ValueToInt256("-12345678910", &out));
  EXPECT_EQ(out, int256_t(-12345678910));

  // Max int256 value can be represented
  int256_t expected_val = kMax256BitInt;
  ASSERT_TRUE(
      Base10ValueToInt256("5789604461865809771178549250434395392663499233282028"
                          "2019728792003956564819967",
                          &out));
  EXPECT_EQ(out, expected_val);

  // Min int256 value can be represented
  expected_val = kMin256BitInt;
  ASSERT_TRUE(Base10ValueToInt256(
      "-5789604461865809771178549250434395392663499233282028"
      "2019728792003956564819968",
      &out));
  EXPECT_EQ(out, expected_val);

  // Should return false when out of bounds
  EXPECT_FALSE(
      Base10ValueToInt256("5789604461865809771178549250434395392663499233282028"
                          "2019728792003956564819968",
                          &out));

  EXPECT_FALSE(Base10ValueToInt256(
      "-5789604461865809771178549250434395392663499233282028"
      "2019728792003956564819969",
      &out));

  EXPECT_FALSE(Base10ValueToInt256("0xB", &out));

  // Check padded values too
  ASSERT_TRUE(Base10ValueToInt256("0000000000000000000000010", &out));
  EXPECT_EQ(out, int256_t(10));
}

}  // namespace brave_wallet
