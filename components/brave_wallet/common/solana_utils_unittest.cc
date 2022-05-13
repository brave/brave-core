/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/solana_utils.h"

#include <map>
#include <vector>

#include "base/test/gtest_util.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
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

TEST(SolanaUtilsUnitTest, Base58Decode) {
  std::vector<uint8_t> account_bytes;
  std::vector<uint8_t> program_bytes;
  std::vector<uint8_t> recent_blockhash_bytes;
  std::string account = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
  std::string program = kSolanaSystemProgramId;
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

  EXPECT_TRUE(Base58Decode(recent_blockhash, &recent_blockhash_bytes,
                           kSolanaBlockhashSize));
  EXPECT_EQ(recent_blockhash_bytes,
            std::vector<uint8_t>({131, 191, 83,  201, 108, 193, 222, 255,
                                  176, 67,  136, 209, 219, 42,  6,   169,
                                  240, 137, 142, 185, 169, 6,   17,  87,
                                  123, 6,   42,  55,  162, 64,  120, 91}));

  // Only exact length should return true.
  std::string two_bytes_encoded = Base58Encode({0, 0});
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(Base58Decode(two_bytes_encoded, &bytes, 2));
  EXPECT_FALSE(Base58Decode(two_bytes_encoded, &bytes, 1));
  EXPECT_FALSE(Base58Decode(two_bytes_encoded, &bytes, 3));

  EXPECT_DCHECK_DEATH(Base58Decode(program, nullptr, kSolanaPubkeySize));
}

TEST(SolanaUtilsUnitTest, IsBase58EncodedSolanaPubkey) {
  EXPECT_TRUE(IsBase58EncodedSolanaPubkey(
      "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw"));
  EXPECT_TRUE(IsBase58EncodedSolanaPubkey(kSolanaSystemProgramId));
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

}  // namespace brave_wallet
