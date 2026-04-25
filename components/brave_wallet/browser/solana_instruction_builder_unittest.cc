/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_builder.h"

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet::solana {

namespace system_program {

TEST(SolanaInstructionBuilderUnitTest, TransferSOL) {
  auto ins = Transfer("pubkey1", "pubkey2", 10000000);
  EXPECT_TRUE(ins);
  EXPECT_EQ(
      ins.value(),
      SolanaInstruction(
          mojom::kSolanaSystemProgramId,
          {SolanaAccountMeta("pubkey1", std::nullopt, true, true),
           SolanaAccountMeta("pubkey2", std::nullopt, false, true)},
          std::vector<uint8_t>({2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0})));

  ins = Transfer("", "", 10000000);
  EXPECT_FALSE(ins);
}

}  // namespace system_program

namespace spl_token_program {

TEST(SolanaInstructionBuilderUnitTest, TransferSPLToken) {
  auto ins =
      TransferChecked("program", "source", "mint_address", "destination",
                      "authority", std::vector<std::string>(), 10000000, 2u);
  EXPECT_TRUE(ins);
  EXPECT_EQ(ins.value(),
            SolanaInstruction(
                "program",
                {SolanaAccountMeta("source", std::nullopt, false, true),
                 SolanaAccountMeta("mint_address", std::nullopt, false, false),
                 SolanaAccountMeta("destination", std::nullopt, false, true),
                 SolanaAccountMeta("authority", std::nullopt, true, false)},
                std::vector<uint8_t>({12, 128, 150, 152, 0, 0, 0, 0, 0, 2})));

  ins = TransferChecked(
      "program", "source", "mint_address", "destination", "authority",
      std::vector<std::string>({"signer1", "signer2"}), 10000000, 2u);
  EXPECT_TRUE(ins);
  EXPECT_EQ(ins.value(),
            SolanaInstruction(
                "program",
                {SolanaAccountMeta("source", std::nullopt, false, true),
                 SolanaAccountMeta("mint_address", std::nullopt, false, false),
                 SolanaAccountMeta("destination", std::nullopt, false, true),
                 SolanaAccountMeta("authority", std::nullopt, false, false),
                 SolanaAccountMeta("signer1", std::nullopt, true, false),
                 SolanaAccountMeta("signer2", std::nullopt, true, false)},
                std::vector<uint8_t>({12, 128, 150, 152, 0, 0, 0, 0, 0, 2})));

  ins = TransferChecked("program", "source", "mint_address", "destination",
                        "authority", std::vector<std::string>(), 1, 2);
  EXPECT_TRUE(ins);
  EXPECT_EQ(ins.value(),
            SolanaInstruction(
                "program",
                {SolanaAccountMeta("source", std::nullopt, false, true),
                 SolanaAccountMeta("mint_address", std::nullopt, false, false),
                 SolanaAccountMeta("destination", std::nullopt, false, true),
                 SolanaAccountMeta("authority", std::nullopt, true, false)},
                std::vector<uint8_t>({12, 1, 0, 0, 0, 0, 0, 0, 0, 2})));

  ins = TransferChecked("", "", "", "", "", std::vector<std::string>(), 1, 2);
  EXPECT_FALSE(ins);
}

}  // namespace spl_token_program

namespace spl_associated_token_account_program {

TEST(SolanaInstructionBuilderUnitTest, CreateAssociatedTokenAccount) {
  auto ins = CreateAssociatedTokenAccount(
      "program", "funding_address", "wallet_address",
      "associated_token_account_address", "spl_token_mint_address");
  ASSERT_TRUE(ins);
  EXPECT_EQ(
      ins.value(),
      SolanaInstruction(
          mojom::kSolanaAssociatedTokenProgramId,
          {SolanaAccountMeta("funding_address", std::nullopt, true, true),
           SolanaAccountMeta("associated_token_account_address", std::nullopt,
                             false, true),
           SolanaAccountMeta("wallet_address", std::nullopt, false, false),
           SolanaAccountMeta("spl_token_mint_address", std::nullopt, false,
                             false),
           SolanaAccountMeta(mojom::kSolanaSystemProgramId, std::nullopt, false,
                             false),
           SolanaAccountMeta("program", std::nullopt, false, false)},
          std::vector<uint8_t>()));

  EXPECT_FALSE(CreateAssociatedTokenAccount("", "funding_address",
                                            "associated_token_account_address",
                                            "wallet_address", ""));

  EXPECT_FALSE(CreateAssociatedTokenAccount("program", "funding_address",
                                            "associated_token_account_address",
                                            "wallet_address", ""));
  EXPECT_FALSE(CreateAssociatedTokenAccount("program", "funding_address", "",
                                            "wallet_address",
                                            "spl_token_mint_address"));
  EXPECT_FALSE(CreateAssociatedTokenAccount("program", "funding_address",
                                            "associated_token_account_address",
                                            "", "spl_token_mint_address"));
  EXPECT_FALSE(CreateAssociatedTokenAccount("program", "funding_address",
                                            "associated_token_account_address",
                                            "wallet_address", ""));
}

}  // namespace spl_associated_token_account_program

namespace compute_budget_program {

TEST(SolanaInstructionBuilderUnitTest, SetComputeUnitLimit) {
  auto instruction = SetComputeUnitLimit(1);
  std::vector<uint8_t> expected_data1 = {2, 1, 0, 0, 0};
  EXPECT_EQ(instruction,
            SolanaInstruction(mojom::kSolanaComputeBudgetProgramId, {},
                              base::span<const uint8_t>(expected_data1)));

  instruction = SetComputeUnitLimit(99);
  std::vector<uint8_t> expected_data2 = {2, 99, 0, 0, 0};
  EXPECT_EQ(instruction,
            SolanaInstruction(mojom::kSolanaComputeBudgetProgramId, {},
                              base::span<const uint8_t>(expected_data2)));
}

TEST(SolanaInstructionBuilderUnitTest, SetComputeUnitPrice) {
  auto instruction = SetComputeUnitPrice(1);
  std::vector<uint8_t> expected_data1 = {3, 1, 0, 0, 0, 0, 0, 0, 0};
  EXPECT_EQ(instruction,
            SolanaInstruction(mojom::kSolanaComputeBudgetProgramId, {},
                              base::span<const uint8_t>(expected_data1)));

  instruction = SetComputeUnitPrice(99);
  std::vector<uint8_t> expected_data2 = {3, 99, 0, 0, 0, 0, 0, 0, 0};
  EXPECT_EQ(instruction,
            SolanaInstruction(mojom::kSolanaComputeBudgetProgramId, {},
                              base::span<const uint8_t>(expected_data2)));
}

}  // namespace compute_budget_program

}  // namespace brave_wallet::solana
