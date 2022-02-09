/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_utils.h"

#include <map>
#include <vector>

#include "base/test/gtest_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
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

  EXPECT_DCHECK_DEATH(Base58Decode(program, nullptr, kSolanaPubkeySize));
}

}  // namespace brave_wallet
