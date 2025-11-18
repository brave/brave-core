/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/hex_utils.h"

#include <limits>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(HexUtilsUnitTest, ToHex) {
  const std::string_view str = "hello world";
  ASSERT_EQ(ToHex(""), "0x0");
  ASSERT_EQ(ToHex(std::string(str)), "0x68656c6c6f20776f726c64");
  ASSERT_EQ(ToHex(base::as_byte_span(str)), "0x68656c6c6f20776f726c64");

  ASSERT_EQ(ToHex(std::vector<uint8_t>()), "0x0");
  ASSERT_EQ(ToHex(std::vector<uint8_t>(str.begin(), str.end())),
            "0x68656c6c6f20776f726c64");
}

TEST(HexUtilsUnitTest, HexEncodeLower) {
  std::string test_string = "hello world";
  ASSERT_EQ(base::HexEncodeLower(test_string), "68656c6c6f20776f726c64");
}

TEST(HexUtilsUnitTest, IsValidHexString) {
  ASSERT_TRUE(IsValidHexString("0x"));
  ASSERT_TRUE(IsValidHexString("0x0"));
  ASSERT_TRUE(IsValidHexString("0x4e02f254184E904300e0775E4b8eeCB14a1b29f0"));
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
  ASSERT_TRUE(PadHexEncodedParameter("0x", &out));
  ASSERT_EQ(
      out,
      "0x0000000000000000000000000000000000000000000000000000000000000000");
  // Invalid input
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
  ASSERT_TRUE(ConcatHexStrings("0x00", "0x00", &out));
  ASSERT_EQ(out, "0x0000");
  ASSERT_TRUE(ConcatHexStrings("0x", "0x", &out));
  ASSERT_EQ(out, "0x");
  ASSERT_TRUE(ConcatHexStrings("0x0", "0x", &out));
  ASSERT_EQ(out, "0x0");
  ASSERT_TRUE(ConcatHexStrings("0x", "0x0", &out));
  ASSERT_EQ(out, "0x0");
  // Invalid input
  ASSERT_FALSE(ConcatHexStrings("0x0", "0", &out));
}

TEST(HexUtilsUnitTest, HexValueToUint256) {
  uint256_t out;
  ASSERT_TRUE(HexValueToUint256("0x", &out));
  ASSERT_EQ(out, (uint256_t)0);
  ASSERT_TRUE(HexValueToUint256("0x0", &out));
  ASSERT_EQ(out, (uint256_t)0);
  ASSERT_TRUE(HexValueToUint256("0x1", &out));
  ASSERT_EQ(out, (uint256_t)1);
  ASSERT_TRUE(HexValueToUint256("0x1234", &out));
  ASSERT_EQ(out, (uint256_t)4660);
  ASSERT_TRUE(HexValueToUint256("0xB", &out));
  ASSERT_EQ(out, (uint256_t)11);

  // Max uint256 value can be represented
  uint256_t expected_val = std::numeric_limits<uint256_t>::max();
  ASSERT_TRUE(HexValueToUint256(
      "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
      &out));
  ASSERT_TRUE(out == (uint256_t)expected_val);

  // Should return false when out of bounds
  ASSERT_FALSE(HexValueToUint256(
      "0x10000000000000000000000000000000000000000000000000000000000000000",
      &out));

  // Check padded values too
  ASSERT_TRUE(HexValueToUint256("0x00000000000000000000000F0", &out));
  ASSERT_EQ(out, (uint256_t)240);
}

TEST(HexUtilsUnitTest, HexValueToInt256) {
  int256_t out;
  ASSERT_TRUE(HexValueToInt256("0x", &out));
  EXPECT_EQ(out, int256_t(0));
  ASSERT_TRUE(HexValueToInt256("0x0", &out));
  EXPECT_EQ(out, int256_t(0));
  ASSERT_TRUE(HexValueToInt256("0x1", &out));
  EXPECT_EQ(out, int256_t(1));
  ASSERT_TRUE(HexValueToInt256("0x1234", &out));
  EXPECT_EQ(out, (int256_t)4660);
  ASSERT_TRUE(HexValueToInt256("0xB", &out));
  EXPECT_EQ(out, (int256_t)11);

  // Max int256 value can be represented
  int256_t expected_val = kMax256BitInt;
  ASSERT_TRUE(HexValueToInt256(
      "0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
      &out));
  EXPECT_EQ(out, expected_val);

  // Min int256 value can be represented
  expected_val = kMin256BitInt;
  ASSERT_TRUE(HexValueToInt256(
      "0x8000000000000000000000000000000000000000000000000000000000000000",
      &out));
  EXPECT_EQ(out, expected_val);

  // Biggest int256 negative value can be represented
  expected_val = int256_t(-1);
  ASSERT_TRUE(HexValueToInt256(
      "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
      &out));
  EXPECT_EQ(out, expected_val);

  // Should return false when out of bounds
  ASSERT_FALSE(HexValueToInt256(
      "0x10000000000000000000000000000000000000000000000000000000000000000",
      &out));

  // Check padded values too
  ASSERT_TRUE(HexValueToInt256("0x00000000000000000000000F0", &out));
  EXPECT_EQ(out, int256_t(240));
}

TEST(HexUtilsUnitTest, Uint256ValueToHex) {
  EXPECT_EQ(Uint256ValueToHex(0), "0x0");
  EXPECT_EQ(Uint256ValueToHex(1), "0x1");
  EXPECT_EQ(Uint256ValueToHex(15), "0xf");
  EXPECT_EQ(Uint256ValueToHex(4660), "0x1234");
  EXPECT_EQ(Uint256ValueToHex(11), "0xb");
  // "10240000000000000000000000"
  uint256_t input_val = 102400000000000;
  input_val *= static_cast<uint256_t>(100000000000);
  EXPECT_EQ(Uint256ValueToHex(input_val), "0x878678326eac900000000");
  EXPECT_EQ(Uint256ValueToHex(3735928559), "0xdeadbeef");
  EXPECT_EQ(
      Uint256ValueToHex(
          0x0000BEEFCAFEBABE'DEADF00DABCDEF89'1234567898765432'F00DCAFED00DFABAu__wb),
      "0xbeefcafebabedeadf00dabcdef891234567898765432f00dcafed00dfaba");
  EXPECT_EQ(
      Uint256ValueToHex(
          0x0001BEEFCAFEBABE'DEADF00DABCDEF89'1234567898765432'F00DCAFED00DFABAu__wb),
      "0x1beefcafebabedeadf00dabcdef891234567898765432f00dcafed00dfaba");
  EXPECT_EQ(
      Uint256ValueToHex(
          0x0FFFFFFFFFFFFFFF'FFFFFFFFFFFFFFFF'FFFFFFFFFFFFFFFF'FFFFFFFFFFFFFFFFu__wb),
      "0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
  EXPECT_EQ(
      Uint256ValueToHex(
          0xEFFFFFFFFFFFFFFF'FFFFFFFFFFFFFFFF'FFFFFFFFFFFFFFFF'FFFFFFFFFFFFFFFFu__wb),
      "0xefffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
  EXPECT_EQ(
      Uint256ValueToHex(
          0xFFFFFFFFFFFFFFFF'FFFFFFFFFFFFFFFF'FFFFFFFFFFFFFFFF'FFFFFFFFFFFFFFFFu__wb),
      "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
  EXPECT_EQ(
      Uint256ValueToHex(
          0xf000000000000000'0000000000000000'0000000000000000'0000000000000000u__wb),
      "0xf000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(
      Uint256ValueToHex(
          0x1000000000000000'0000000000000000'0000000000000000'0000000000000000u__wb),
      "0x1000000000000000000000000000000000000000000000000000000000000000");
}

TEST(HexUtilsUnitTest, PrefixedHexStringToBytes) {
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(PrefixedHexStringToBytes("0x", &bytes));
  EXPECT_EQ(bytes, (std::vector<uint8_t>()));
  EXPECT_TRUE(PrefixedHexStringToBytes("0x0", &bytes));
  EXPECT_EQ(bytes, (std::vector<uint8_t>{0}));
  EXPECT_TRUE(PrefixedHexStringToBytes("0x00", &bytes));
  EXPECT_EQ(bytes, (std::vector<uint8_t>{0}));
  EXPECT_TRUE(PrefixedHexStringToBytes("0x1", &bytes));
  EXPECT_EQ(bytes, (std::vector<uint8_t>{1}));
  EXPECT_TRUE(PrefixedHexStringToBytes("0xdeadbeef", &bytes));
  EXPECT_EQ(bytes, (std::vector<uint8_t>{222, 173, 190, 239}));
  EXPECT_FALSE(PrefixedHexStringToBytes("0x0g", &bytes));
  EXPECT_FALSE(PrefixedHexStringToBytes("hello", &bytes));
  EXPECT_FALSE(PrefixedHexStringToBytes("01", &bytes));
  EXPECT_FALSE(PrefixedHexStringToBytes("", &bytes));

  EXPECT_EQ(PrefixedHexStringToBytes("0x"), (std::vector<uint8_t>()));
  EXPECT_EQ(PrefixedHexStringToBytes("0x0"), (std::vector<uint8_t>{0}));
  EXPECT_EQ(PrefixedHexStringToBytes("0x00"), (std::vector<uint8_t>{0}));
  EXPECT_EQ(PrefixedHexStringToBytes("0x1"), (std::vector<uint8_t>{1}));
  EXPECT_EQ(PrefixedHexStringToBytes("0xdeadbeef"),
            (std::vector<uint8_t>{222, 173, 190, 239}));
  EXPECT_FALSE(PrefixedHexStringToBytes("0x0g"));
  EXPECT_FALSE(PrefixedHexStringToBytes("hello"));
  EXPECT_FALSE(PrefixedHexStringToBytes("01"));
  EXPECT_FALSE(PrefixedHexStringToBytes(""));
}

TEST(HexUtilsUnitTest, PrefixedHexStringToFixed) {
  std::array<uint8_t, 0> out0 = {};
  std::array<uint8_t, 1> out1 = {};
  std::array<uint8_t, 2> out2 = {};
  std::array<uint8_t, 4> out4 = {};
  std::array<uint8_t, 8> out8 = {};
  std::array<uint8_t, 16> out16 = {};
  std::array<uint8_t, 32> out32 = {};

  // Empty output spans, should unconditionally return false.
  EXPECT_FALSE(PrefixedHexStringToFixed("", out0));
  EXPECT_FALSE(PrefixedHexStringToFixed("0x", out0));
  EXPECT_FALSE(PrefixedHexStringToFixed("0xy", out0));
  EXPECT_FALSE(PrefixedHexStringToFixed("0xxy", out0));
  EXPECT_FALSE(PrefixedHexStringToFixed("0x123", out0));
  EXPECT_FALSE(PrefixedHexStringToFixed("0x1234", out0));

  // Empty strings, should fail.
  EXPECT_FALSE(PrefixedHexStringToFixed("0x", out1));
  EXPECT_FALSE(PrefixedHexStringToFixed("", out1));

  // Invalid hex digits, but correct lengths.
  EXPECT_FALSE(PrefixedHexStringToFixed("0xy", out1));
  EXPECT_FALSE(PrefixedHexStringToFixed("0xxy", out1));

  // No leading 0x marker, but correct lengths.
  EXPECT_FALSE(PrefixedHexStringToFixed("0", out1));
  EXPECT_FALSE(PrefixedHexStringToFixed("00", out1));
  EXPECT_FALSE(PrefixedHexStringToFixed("123", out2));
  EXPECT_FALSE(PrefixedHexStringToFixed("0123", out2));

  // Length mismatch but otherwise valid hex strings.
  EXPECT_FALSE(PrefixedHexStringToFixed("0x11223", out4));
  EXPECT_FALSE(PrefixedHexStringToFixed("0x112233", out4));
  EXPECT_FALSE(PrefixedHexStringToFixed("0x112233445", out4));
  EXPECT_FALSE(PrefixedHexStringToFixed("0x1122334455", out4));
  EXPECT_FALSE(PrefixedHexStringToFixed("0x1", out16));
  EXPECT_FALSE(PrefixedHexStringToFixed("0x11", out16));

  EXPECT_TRUE(PrefixedHexStringToFixed("0x0", out1));
  EXPECT_EQ(out1, (std::array<uint8_t, 1>{0x00}));

  EXPECT_TRUE(PrefixedHexStringToFixed("0x01", out1));
  EXPECT_EQ(out1, (std::array<uint8_t, 1>{0x01}));

  EXPECT_TRUE(PrefixedHexStringToFixed("0xf", out1));
  EXPECT_EQ(out1, (std::array<uint8_t, 1>{0x0f}));

  EXPECT_TRUE(PrefixedHexStringToFixed("0x3", out1));
  EXPECT_EQ(out1, (std::array<uint8_t, 1>{0x03}));

  EXPECT_TRUE(PrefixedHexStringToFixed("0x0123", out2));
  EXPECT_EQ(out2, (std::array<uint8_t, 2>{0x01, 0x23}));

  EXPECT_TRUE(PrefixedHexStringToFixed("0x123", out2));
  EXPECT_EQ(out2, (std::array<uint8_t, 2>{0x01, 0x23}));

  EXPECT_TRUE(PrefixedHexStringToFixed("0xdeadbeef", out4));
  EXPECT_EQ(out4, (std::array<uint8_t, 4>{0xde, 0xad, 0xbe, 0xef}));

  EXPECT_TRUE(PrefixedHexStringToFixed("0x0123456789abcdef", out8));
  EXPECT_EQ(out8, (std::array<uint8_t, 8>{0x01, 0x23, 0x45, 0x67, 0x89, 0xab,
                                          0xcd, 0xef}));

  EXPECT_TRUE(PrefixedHexStringToFixed("0xfedcba9876543210", out8));
  EXPECT_EQ(out8, (std::array<uint8_t, 8>{0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54,
                                          0x32, 0x10}));

  std::string_view hash =
      "0xba38d3e0e1033e97a3aa294e59741c9f4ab8786c8d55c493d0ebc58b885961b3";
  EXPECT_TRUE(PrefixedHexStringToFixed(hash, out32));
  EXPECT_EQ(base::HexEncode(out32),
            "BA38D3E0E1033E97A3AA294E59741C9F4AB8786C8D55C493D0EBC58B885961B3");
}

}  // namespace brave_wallet
