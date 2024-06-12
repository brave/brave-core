/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/solana_utils.h"

#include <map>
#include <tuple>
#include <vector>

#include "base/test/gtest_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SolanaUtilsUnitTest, CompactU16Encode) {
  std::map<uint16_t, std::vector<uint8_t>> input_output_map = {
      {0x0, {0x0}},
      {0x7f, {0x7f}},
      {0x80, {0x80, 0x01}},
      {0xff, {0xff, 0x01}},
      {0x100, {0x80, 0x02}},
      {0x7fff, {0xff, 0xff, 0x01}},
      {0xffff, {0xff, 0xff, 0x03}}};
  for (const auto& kv : input_output_map) {
    std::vector<uint8_t> out;
    CompactU16Encode(kv.first, &out);
    EXPECT_EQ(out, kv.second);
  }

  EXPECT_DCHECK_DEATH(CompactU16Encode(0x0, nullptr));
}

// Test cases are from
// https://github.com/solana-labs/solana/blob/79df1954eb5e8d951d2dd2b5ea094475d18551db/sdk/program/src/short_vec.rs#L312
TEST(SolanaUtilsUnitTest, CompactU16Decode) {
  std::map<std::tuple<uint16_t, size_t>, std::vector<uint8_t>> valid_map = {
      {std::tuple<uint16_t, size_t>(0x0000, 1), {0x00}},
      {std::tuple<uint16_t, size_t>(0x007f, 1), {0x7f}},
      {std::tuple<uint16_t, size_t>(0x0080, 2), {0x80, 0x01}},
      {std::tuple<uint16_t, size_t>(0x00ff, 2), {0xff, 0x01}},
      {std::tuple<uint16_t, size_t>(0x0100, 2), {0x80, 0x02}},
      {std::tuple<uint16_t, size_t>(0x07ff, 2), {0xff, 0x0f}},
      {std::tuple<uint16_t, size_t>(0x3fff, 2), {0xff, 0x7f}},
      {std::tuple<uint16_t, size_t>(0x4000, 3), {0x80, 0x80, 0x01}},
      {std::tuple<uint16_t, size_t>(0xffff, 3), {0xff, 0xff, 0x03}}};

  for (const auto& kv : valid_map) {
    auto out = CompactU16Decode(kv.second, 0);
    EXPECT_EQ(*out, kv.first);
  }

  // Test start_index != 0 and extra data (not part of this u16) in the byte
  // array.
  std::tuple<uint16_t, size_t> expect_out = std::make_tuple(0x00ff, 2);
  EXPECT_EQ(*CompactU16Decode({0x00, 0xff, 0x01, 0x80}, 1), expect_out);

  std::vector<std::vector<uint8_t>> invalid_cases = {{0x80, 0x00},
                                                     {0x80, 0x80, 0x00},
                                                     {0xff, 0x00},
                                                     {0xff, 0x80, 0x00},
                                                     {0x80, 0x81, 0x00},
                                                     {0xff, 0x81, 0x00},
                                                     {0x80, 0x82, 0x00},
                                                     {0xff, 0x8f, 0x00},
                                                     {0xff, 0xff, 0x00},
                                                     // too short
                                                     {},
                                                     {0x80},
                                                     // too long
                                                     {0x80, 0x80, 0x80, 0x00},
                                                     // too large
                                                     {0x80, 0x80, 0x04},
                                                     {0x80, 0x80, 0x06}};

  for (size_t i = 0; i < invalid_cases.size(); ++i) {
    EXPECT_FALSE(CompactU16Decode(invalid_cases[i], 0)) << i;
  }
}

TEST(SolanaUtilsUnitTest, Base58Decode) {
  std::vector<uint8_t> account_bytes;
  std::vector<uint8_t> program_bytes;
  std::vector<uint8_t> recent_blockhash_bytes;
  std::string account = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
  std::string program = mojom::kSolanaSystemProgramId;
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";

  EXPECT_TRUE(Base58Decode(account, &account_bytes, kSolanaPubkeySize));
  EXPECT_EQ(account_bytes,
            std::vector<uint8_t>({34,  208, 53,  54,  75,  46,  112, 55,
                                  123, 15,  232, 9,   45,  178, 252, 196,
                                  62,  64,  169, 213, 66,  87,  192, 16,
                                  152, 108, 254, 148, 183, 39,  51,  192}));

  EXPECT_TRUE(Base58Decode(program, &program_bytes, kSolanaPubkeySize));
  EXPECT_EQ(
      program_bytes,
      std::vector<uint8_t>({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));

  EXPECT_TRUE(
      Base58Decode(recent_blockhash, &recent_blockhash_bytes, kSolanaHashSize));
  EXPECT_EQ(recent_blockhash_bytes,
            std::vector<uint8_t>({131, 191, 83,  201, 108, 193, 222, 255,
                                  176, 67,  136, 209, 219, 42,  6,   169,
                                  240, 137, 142, 185, 169, 6,   17,  87,
                                  123, 6,   42,  55,  162, 64,  120, 91}));

  // Only exact length should return true with strict default to true.
  std::string two_bytes_encoded = Base58Encode({0, 0});
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(Base58Decode(two_bytes_encoded, &bytes, 2));
  EXPECT_FALSE(Base58Decode(two_bytes_encoded, &bytes, 1));
  EXPECT_FALSE(Base58Decode(two_bytes_encoded, &bytes, 3));

  // Len would be treated as max_len when strict is false.
  EXPECT_TRUE(Base58Decode(two_bytes_encoded, &bytes, 3, false));

  EXPECT_DCHECK_DEATH(Base58Decode(program, nullptr, kSolanaPubkeySize));
}

TEST(SolanaUtilsUnitTest, IsBase58EncodedSolanaPubkey) {
  EXPECT_TRUE(IsBase58EncodedSolanaPubkey(
      "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw"));
  EXPECT_TRUE(IsBase58EncodedSolanaPubkey(mojom::kSolanaSystemProgramId));
  EXPECT_TRUE(IsBase58EncodedSolanaPubkey(
      Base58Encode(std::vector<uint8_t>(kSolanaPubkeySize, 0))));

  EXPECT_FALSE(IsBase58EncodedSolanaPubkey(""));
  EXPECT_FALSE(IsBase58EncodedSolanaPubkey(
      "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8"));
  // Incorrect sizes.
  EXPECT_FALSE(IsBase58EncodedSolanaPubkey(
      Base58Encode(std::vector<uint8_t>(kSolanaPubkeySize + 1, 0))));
  EXPECT_FALSE(IsBase58EncodedSolanaPubkey(
      Base58Encode(std::vector<uint8_t>(kSolanaPubkeySize - 1, 0))));
}

TEST(SolanaUtilsUnitTest, Uint8ArrayDecode) {
  const struct {
    const char* input;
    const size_t len;
    const std::vector<uint8_t> output;
  } valid_cases[] = {{"[34,  208, 53,  54,  75,  46,  112, 55]", 8,
                      std::vector<uint8_t>({34, 208, 53, 54, 75, 46, 112, 55})},
                     {"[0, 0, 0, 0]", 4, std::vector<uint8_t>({0, 0, 0, 0})},
                     {"[34,  208, 53,  54,  75,  46,  112, 55, 123, 15,  232, "
                      "9,   45,  178, 252, 196, 62,  64,  169, 213, 66,  87,  "
                      "192, 16, 152, 108, 254, 148, 183, 39,  51,  192]",
                      kSolanaPubkeySize,
                      std::vector<uint8_t>(
                          {34,  208, 53,  54,  75,  46,  112, 55,  123, 15, 232,
                           9,   45,  178, 252, 196, 62,  64,  169, 213, 66, 87,
                           192, 16,  152, 108, 254, 148, 183, 39,  51,  192})}};

  for (const auto& valid_case : valid_cases) {
    SCOPED_TRACE(valid_case.input);
    std::vector<uint8_t> bytes;
    EXPECT_TRUE(Uint8ArrayDecode(valid_case.input, &bytes, valid_case.len));
    EXPECT_EQ(bytes, valid_case.output);
  }

  const struct {
    const char* input;
    const size_t len;
  } invalid_cases[] = {{"[]", 0},
                       {"[,]", 0},
                       {"", 0},
                       {"[", 0},
                       {"]", 0},
                       {"[34]", 0},
                       {"[34}", 1},
                       {"{34]", 1},
                       {"[34:208:53]", 3},
                       {"[34, HELLO]", 2},
                       {"[34, 256]", 2},
                       {"[34, 208, 53, 43]", 8},
                       {"44, [34, 208, 53, 43]", 5}};
  for (const auto& invalid_case : invalid_cases) {
    SCOPED_TRACE(invalid_case.input);
    std::vector<uint8_t> bytes;
    EXPECT_FALSE(
        Uint8ArrayDecode(invalid_case.input, &bytes, invalid_case.len));
    EXPECT_TRUE(bytes.empty());
  }
}

TEST(SolanaUtilsUnitTest, CompactArrayDecode) {
  std::vector<uint8_t> bytes = {0, 1, 2, 5, 4, 6, 7, 8};
  size_t start_index = 2;
  auto ret_bytes = CompactArrayDecode(bytes, &start_index);
  ASSERT_TRUE(ret_bytes);
  EXPECT_EQ(*ret_bytes, std::vector<uint8_t>({5, 4}));
  EXPECT_EQ(start_index, 5u);

  // Test out-of-bound, array length is 6 but only two bytes {7, 8} left.
  EXPECT_FALSE(CompactArrayDecode(bytes, &start_index));
}

TEST(SolanaUtilsUnitTest, GetUint8FromStringDict) {
  base::Value::Dict dict;
  dict.Set("valid", base::NumberToString(UINT8_MAX));
  dict.Set("too_big", base::NumberToString(UINT8_MAX + 1));
  dict.Set("not_a_number", "HELLO");
  EXPECT_EQ(GetUint8FromStringDict(dict, "valid").value(), UINT8_MAX);
  EXPECT_FALSE(GetUint8FromStringDict(dict, "too_big"));
  EXPECT_FALSE(GetUint8FromStringDict(dict, "not_a_number"));
}

}  // namespace brave_wallet
