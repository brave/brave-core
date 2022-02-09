/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_builder.h"

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace solana {

namespace system_program {

TEST(SolanaInstructionBuilderUnitTest, TransferSOL) {
  auto ins = Transfer("pubkey1", "pubkey2", 10000000);
  EXPECT_TRUE(ins);
  EXPECT_EQ(ins.value(),
            SolanaInstruction(kSolanaSystemProgramId,
                              {SolanaAccountMeta("pubkey1", true, true),
                               SolanaAccountMeta("pubkey2", false, true)},
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
            SolanaInstruction("program",
                              {SolanaAccountMeta("source", false, true),
                               SolanaAccountMeta("destination", false, true),
                               SolanaAccountMeta("authority", true, false)},
                              {3, 128, 150, 152, 0, 0, 0, 0, 0}));

  ins = Transfer("program", "source", "destination", "authority",
                 std::vector<std::string>({"signer1", "signer2"}), 10000000);
  EXPECT_TRUE(ins);
  EXPECT_EQ(ins.value(),
            SolanaInstruction("program",
                              {SolanaAccountMeta("source", false, true),
                               SolanaAccountMeta("destination", false, true),
                               SolanaAccountMeta("authority", false, false),
                               SolanaAccountMeta("signer1", true, false),
                               SolanaAccountMeta("signer2", true, false)},
                              {3, 128, 150, 152, 0, 0, 0, 0, 0}));

  ins = Transfer("program", "source", "destination", "authority",
                 std::vector<std::string>(), 1);
  EXPECT_TRUE(ins);
  EXPECT_EQ(ins.value(),
            SolanaInstruction("program",
                              {SolanaAccountMeta("source", false, true),
                               SolanaAccountMeta("destination", false, true),
                               SolanaAccountMeta("authority", true, false)},
                              {3, 1, 0, 0, 0, 0, 0, 0, 0}));

  ins = Transfer("", "", "", "", std::vector<std::string>(), 1);
  EXPECT_FALSE(ins);
}

}  // namespace spl_token_program

}  // namespace solana

}  // namespace brave_wallet
