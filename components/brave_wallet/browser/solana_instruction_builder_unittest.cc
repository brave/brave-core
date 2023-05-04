/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_builder.h"

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace solana {

namespace system_program {

TEST(SolanaInstructionBuilderUnitTest, TransferSOL) {
  auto ins = Transfer("pubkey1", "pubkey2", 10000000);
  EXPECT_TRUE(ins);
  EXPECT_EQ(ins.value(),
            SolanaInstruction(
                mojom::kSolanaSystemProgramId,
                {SolanaAccountMeta("pubkey1", absl::nullopt, true, true),
                 SolanaAccountMeta("pubkey2", absl::nullopt, false, true)},
                {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0}));

  ins = Transfer("", "", 10000000);
  EXPECT_FALSE(ins);
}

}  // namespace system_program

namespace spl_token_program {

TEST(SolanaInstructionBuilderUnitTest, TransferSPLToken) {
  auto ins = Transfer("program", "source", "destination", "authority",
                      std::vector<std::string>(), 10000000);
  EXPECT_TRUE(ins);
  EXPECT_EQ(ins.value(),
            SolanaInstruction(
                "program",
                {SolanaAccountMeta("source", absl::nullopt, false, true),
                 SolanaAccountMeta("destination", absl::nullopt, false, true),
                 SolanaAccountMeta("authority", absl::nullopt, true, false)},
                {3, 128, 150, 152, 0, 0, 0, 0, 0}));

  ins = Transfer("program", "source", "destination", "authority",
                 std::vector<std::string>({"signer1", "signer2"}), 10000000);
  EXPECT_TRUE(ins);
  EXPECT_EQ(ins.value(),
            SolanaInstruction(
                "program",
                {SolanaAccountMeta("source", absl::nullopt, false, true),
                 SolanaAccountMeta("destination", absl::nullopt, false, true),
                 SolanaAccountMeta("authority", absl::nullopt, false, false),
                 SolanaAccountMeta("signer1", absl::nullopt, true, false),
                 SolanaAccountMeta("signer2", absl::nullopt, true, false)},
                {3, 128, 150, 152, 0, 0, 0, 0, 0}));

  ins = Transfer("program", "source", "destination", "authority",
                 std::vector<std::string>(), 1);
  EXPECT_TRUE(ins);
  EXPECT_EQ(ins.value(),
            SolanaInstruction(
                "program",
                {SolanaAccountMeta("source", absl::nullopt, false, true),
                 SolanaAccountMeta("destination", absl::nullopt, false, true),
                 SolanaAccountMeta("authority", absl::nullopt, true, false)},
                {3, 1, 0, 0, 0, 0, 0, 0, 0}));

  ins = Transfer("", "", "", "", std::vector<std::string>(), 1);
  EXPECT_FALSE(ins);
}

}  // namespace spl_token_program

namespace spl_associated_token_account_program {

TEST(SolanaInstructionBuilderUnitTest, CreateAssociatedTokenAccount) {
  auto ins = CreateAssociatedTokenAccount("funding_address", "wallet_address",
                                          "associated_token_account_address",
                                          "spl_token_mint_address");
  ASSERT_TRUE(ins);
  EXPECT_EQ(
      ins.value(),
      SolanaInstruction(
          mojom::kSolanaAssociatedTokenProgramId,
          {SolanaAccountMeta("funding_address", absl::nullopt, true, true),
           SolanaAccountMeta("associated_token_account_address", absl::nullopt,
                             false, true),
           SolanaAccountMeta("wallet_address", absl::nullopt, false, false),
           SolanaAccountMeta("spl_token_mint_address", absl::nullopt, false,
                             false),
           SolanaAccountMeta(mojom::kSolanaSystemProgramId, absl::nullopt,
                             false, false),
           SolanaAccountMeta(mojom::kSolanaTokenProgramId, absl::nullopt, false,
                             false)},
          std::vector<uint8_t>()));

  EXPECT_FALSE(CreateAssociatedTokenAccount("funding_address",
                                            "associated_token_account_address",
                                            "wallet_address", ""));
  EXPECT_FALSE(CreateAssociatedTokenAccount(
      "funding_address", "", "wallet_address", "spl_token_mint_address"));
  EXPECT_FALSE(CreateAssociatedTokenAccount("funding_address",
                                            "associated_token_account_address",
                                            "", "spl_token_mint_address"));
  EXPECT_FALSE(CreateAssociatedTokenAccount("funding_address",
                                            "associated_token_account_address",
                                            "wallet_address", ""));
}

}  // namespace spl_associated_token_account_program

}  // namespace solana

}  // namespace brave_wallet
