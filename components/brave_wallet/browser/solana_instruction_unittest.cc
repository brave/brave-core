/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction.h"

#include <vector>

#include "base/test/gtest_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SolanaInstructionUnitTest, Serialize) {
  // Test serializing an instruction for transfering SOL from an account to
  // itself.
  std::string from_account = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
  std::string to_account = from_account;

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});

  std::vector<SolanaAccountMeta> account_metas = {
      SolanaAccountMeta(from_account, true, true),
      SolanaAccountMeta(kSolanaSystemProgramId, false, false)};
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(instruction.Serialize(account_metas, &bytes));
  std::vector<uint8_t> expected_bytes = {
      1,                                         // program id index
      2,                                         // length of accounts
      0,  0,                                     // account indices
      12,                                        // data length
      2,  0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0  // data
  };
  EXPECT_EQ(bytes, expected_bytes);

  // Program ID not found.
  EXPECT_FALSE(
      instruction.Serialize({SolanaAccountMeta(from_account, true, true),
                             SolanaAccountMeta(from_account, true, true)},
                            &bytes));

  // Account not found.
  EXPECT_FALSE(instruction.Serialize(
      {SolanaAccountMeta(kSolanaSystemProgramId, false, false),
       SolanaAccountMeta(kSolanaSystemProgramId, false, false)},
      &bytes));

  // Input account meta length > uint8_t max.
  std::vector<SolanaAccountMeta> oversize_account_metas = account_metas;
  for (uint16_t i = 1; i < 256; i++) {
    oversize_account_metas.push_back(account_metas[0]);
  }
  EXPECT_FALSE(instruction.Serialize(oversize_account_metas, &bytes));

  // Instruction account size > input account meta array size.
  EXPECT_FALSE(instruction.Serialize(
      {SolanaAccountMeta(from_account, true, true)}, &bytes));

  // Instruction account size > uint8_t max.
  SolanaInstruction invalid_instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      oversize_account_metas,
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  EXPECT_FALSE(invalid_instruction.Serialize(account_metas, &bytes));

  EXPECT_DCHECK_DEATH(instruction.Serialize({}, nullptr));
}

}  // namespace brave_wallet
