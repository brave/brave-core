/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/hex_utils.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(HexUtilsUnitTest, ToHex) {
  ASSERT_EQ(ToHex(""), "0x0");
  ASSERT_EQ(ToHex("hello world"), "0x68656c6c6f20776f726c64");

  ASSERT_EQ(ToHex(std::vector<uint8_t>()), "0x0");
  const std::string str1("hello world");
  ASSERT_EQ(ToHex(std::vector<uint8_t>(str1.begin(), str1.end())),
            "0x68656c6c6f20776f726c64");
}

TEST(HexUtilsUnitTest, IsValidHexString) {
  ASSERT_TRUE(IsValidHexString("0x0"));
  ASSERT_TRUE(IsValidHexString("0x4e02f254184E904300e0775E4b8eeCB14a1b29f0"));
  ASSERT_FALSE(IsValidHexString("0x"));
  ASSERT_FALSE(IsValidHexString("0xZ"));
  ASSERT_FALSE(IsValidHexString("123"));
  ASSERT_FALSE(IsValidHexString("0"));
  ASSERT_FALSE(IsValidHexString(""));
  ASSERT_FALSE(IsValidHexString("0xBraVe"));
  ASSERT_FALSE(IsValidHexString("0x12$$"));
}

TEST(HexUtilsUnitTest, PadHexEncodedParameter) {
  std::string out;
  // Pad an address
  ASSERT_TRUE(PadHexEncodedParameter(
      "0x4e02f254184E904300e0775E4b8eeCB14a1b29f0", &out));
  ASSERT_EQ(
      out,
      "0x0000000000000000000000004e02f254184E904300e0775E4b8eeCB14a1b29f0");

  // Corner case: 62
  ASSERT_TRUE(PadHexEncodedParameter(
      "0x11111111112222222222333333333344444444445555555555666666666600",
      &out));
  ASSERT_EQ(
      out,
      "0x0011111111112222222222333333333344444444445555555555666666666600");

  ASSERT_TRUE(PadHexEncodedParameter("0x0", &out));
  ASSERT_EQ(
      out,
      "0x0000000000000000000000000000000000000000000000000000000000000000");
  // Invalid input
  ASSERT_FALSE(PadHexEncodedParameter("0x", &out));
  ASSERT_FALSE(PadHexEncodedParameter("0", &out));
  ASSERT_FALSE(PadHexEncodedParameter("", &out));
}

TEST(HexUtilsUnitTest, ConcatHexStrings) {
  std::string out;
  // Pad an address
  ASSERT_TRUE(ConcatHexStrings(
      "0x70a08231",
      "0x0000000000000000000000004e02f254184E904300e0775E4b8eeCB14a1b29f0",
      &out));
  ASSERT_EQ(out,
            "0x70a082310000000000000000000000004e02f254184E904300e0775E4b8eeCB1"
            "4a1b29f0");
  ASSERT_TRUE(ConcatHexStrings("0x0", "0x0", &out));
  ASSERT_EQ(out, "0x00");
  // Invalid input
  ASSERT_FALSE(ConcatHexStrings("0x", "0x0", &out));
  ASSERT_FALSE(ConcatHexStrings("0x0", "0", &out));
}

TEST(HexUtilsUnitTest, HexValueToUint256) {
  uint256_t out;
  ASSERT_TRUE(HexValueToUint256("0x1", &out));
  ASSERT_EQ(out, (uint256_t)1);
  ASSERT_TRUE(HexValueToUint256("0x1234", &out));
  ASSERT_EQ(out, (uint256_t)4660);
  ASSERT_TRUE(HexValueToUint256("0xB", &out));
  ASSERT_EQ(out, (uint256_t)11);
  uint256_t expected_val = 102400000000000;
  // "10240000000000000000000000"
  expected_val *= static_cast<uint256_t>(100000000000);
  ASSERT_TRUE(HexValueToUint256("0x878678326eac900000000", &out));
  ASSERT_TRUE(out == (uint256_t)expected_val);
  // Check padded values too
  ASSERT_TRUE(HexValueToUint256("0x00000000000000000000000F0", &out));
  ASSERT_EQ(out, (uint256_t)240);
}

TEST(HexUtilsUnitTest, Uint256ValueToHex) {
  ASSERT_EQ(Uint256ValueToHex(1), "0x1");
  ASSERT_EQ(Uint256ValueToHex(4660), "0x1234");
  ASSERT_EQ(Uint256ValueToHex(11), "0xb");
  // "10240000000000000000000000"
  uint256_t input_val = 102400000000000;
  input_val *= static_cast<uint256_t>(100000000000);
  ASSERT_EQ(Uint256ValueToHex(input_val), "0x878678326eac900000000");
  ASSERT_EQ(Uint256ValueToHex(3735928559), "0xdeadbeef");
}

}  // namespace brave_wallet
