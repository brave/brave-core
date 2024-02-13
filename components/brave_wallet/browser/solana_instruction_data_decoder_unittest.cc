/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_data_decoder.h"

#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet::solana_ins_data_decoder {

constexpr char kPubkey1[] = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
constexpr char kPubkey2[] = "D3tynVS3dHGoShEZQcSbsJ69DnoWunhcgya35r5Dtn4p";

class SolanaInstructionDecoderTest : public testing::Test {
 public:
  SolanaInstructionDecoderTest() = default;
  ~SolanaInstructionDecoderTest() override = default;

  void TestSystemInstruction(
      const std::vector<uint8_t>& data,
      mojom::SolanaSystemInstruction ins_type,
      const std::vector<std::pair<std::string, std::string>>& params) {
    auto ret = Decode(data, mojom::kSolanaSystemProgramId);
    ASSERT_TRUE(ret);
    EXPECT_EQ(ret->sys_ins_type, ins_type);
    EXPECT_FALSE(ret->token_ins_type);
    ASSERT_EQ(params.size(), ret->params.size());
    for (size_t i = 0; i < params.size(); ++i) {
      EXPECT_EQ(params[i], std::make_pair(std::get<0>(ret->params[i]),
                                          std::get<2>(ret->params[i])));
    }
    EXPECT_EQ(GetAccountParamsForTesting(ins_type, std::nullopt),
              ret->account_params);

    for (size_t i = 0; i < data.size(); ++i) {
      EXPECT_FALSE(Decode(std::vector<uint8_t>(data.begin(), data.begin() + i),
                          mojom::kSolanaSystemProgramId));
    }
  }

  void TestTokenInstruction(
      const std::vector<uint8_t>& data,
      mojom::SolanaTokenInstruction ins_type,
      const std::vector<std::pair<std::string, std::string>>& params,
      bool skip_byte_checks = false) {
    auto ret = Decode(data, mojom::kSolanaTokenProgramId);
    ASSERT_TRUE(ret);
    EXPECT_FALSE(ret->sys_ins_type);
    EXPECT_EQ(ret->token_ins_type, ins_type);
    ASSERT_EQ(params.size(), ret->params.size());
    for (size_t i = 0; i < params.size(); ++i) {
      EXPECT_EQ(params[i], std::make_pair(std::get<0>(ret->params[i]),
                                          std::get<2>(ret->params[i])));
    }
    EXPECT_EQ(GetAccountParamsForTesting(std::nullopt, ins_type),
              ret->account_params);

    // Skip when testing typescript impl without passing the optional key
    // because the padded 32 bytes of 0 for the public key are ignored and
    // won't be checked.
    if (skip_byte_checks) {
      return;
    }
    for (size_t i = 0; i < data.size(); ++i) {
      EXPECT_FALSE(Decode(std::vector<uint8_t>(data.begin(), data.begin() + i),
                          mojom::kSolanaTokenProgramId));
    }
  }
};

TEST_F(SolanaInstructionDecoderTest, Decode_SystemCreateAccount) {
  TestSystemInstruction(
      {0,   0,   0,   0,   16,  39, 0,   0,   0,   0,   0,   0,   100,
       0,   0,   0,   0,   0,   0,  0,   6,   221, 246, 225, 215, 101,
       161, 147, 217, 203, 225, 70, 206, 235, 121, 172, 28,  180, 133,
       237, 95,  91,  55,  145, 58, 140, 245, 133, 126, 255, 0,   169},
      mojom::SolanaSystemInstruction::kCreateAccount,
      {{"lamports", "10000"},
       {"space", "100"},
       {"owner_program", mojom::kSolanaTokenProgramId}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_SystemAssign) {
  TestSystemInstruction(
      {1,   0,   0,   0,   6,   221, 246, 225, 215, 101, 161, 147,
       217, 203, 225, 70,  206, 235, 121, 172, 28,  180, 133, 237,
       95,  91,  55,  145, 58,  140, 245, 133, 126, 255, 0,   169},
      mojom::SolanaSystemInstruction::kAssign,
      {{"owner_program", mojom::kSolanaTokenProgramId}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_SystemTransfer) {
  TestSystemInstruction({2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0},
                        mojom::SolanaSystemInstruction::kTransfer,
                        {{"lamports", "10000000"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_SystemCreateAccountWithSeed) {
  TestSystemInstruction(
      {3,   0,   0,   0,   179, 10,  45,  120, 165, 79,  23,  213, 130,
       206, 38,  194, 56,  107, 31,  15,  105, 52,  170, 204, 201, 218,
       15,  234, 163, 176, 140, 194, 226, 39,  121, 169, 9,   0,   0,
       0,   0,   0,   0,   0,   84,  69,  83,  84,  32,  83,  69,  69,
       68,  16,  39,  0,   0,   0,   0,   0,   0,   100, 0,   0,   0,
       0,   0,   0,   0,   6,   221, 246, 225, 215, 101, 161, 147, 217,
       203, 225, 70,  206, 235, 121, 172, 28,  180, 133, 237, 95,  91,
       55,  145, 58,  140, 245, 133, 126, 255, 0,   169},
      mojom::SolanaSystemInstruction::kCreateAccountWithSeed,
      {{"base", kPubkey2},
       {"seed", "TEST SEED"},
       {"lamports", "10000"},
       {"space", "100"},
       {"owner_program", mojom::kSolanaTokenProgramId}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_SystemAdvanceNonceAccount) {
  TestSystemInstruction(
      {4, 0, 0, 0}, mojom::SolanaSystemInstruction::kAdvanceNonceAccount, {});
}

TEST_F(SolanaInstructionDecoderTest, Decode_SystemWithdrawNonceAccount) {
  TestSystemInstruction({5, 0, 0, 0, 16, 39, 0, 0, 0, 0, 0, 0},
                        mojom::SolanaSystemInstruction::kWithdrawNonceAccount,
                        {{"lamports", "10000"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_SystemInitializeNonceAccount) {
  TestSystemInstruction(
      {6,   0,   0,   0,   161, 51,  89,  91, 115, 210, 217, 212,
       76,  159, 171, 200, 40,  150, 157, 70, 197, 71,  24,  44,
       209, 108, 143, 4,   58,  251, 215, 62, 201, 172, 159, 197},
      mojom::SolanaSystemInstruction::kInitializeNonceAccount,
      {{"authority", kPubkey1}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_SystemAuthorizeNonceAccount) {
  TestSystemInstruction(
      {7,   0,   0,  0,   179, 10,  45,  120, 165, 79, 23,  213,
       130, 206, 38, 194, 56,  107, 31,  15,  105, 52, 170, 204,
       201, 218, 15, 234, 163, 176, 140, 194, 226, 39, 121, 169},
      mojom::SolanaSystemInstruction::kAuthorizeNonceAccount,
      {{"new_authority", kPubkey2}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_SystemAllocate) {
  TestSystemInstruction({8, 0, 0, 0, 16, 39, 0, 0, 0, 0, 0, 0},
                        mojom::SolanaSystemInstruction::kAllocate,
                        {{"space", "10000"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_SystemAllocateWithSeed) {
  TestSystemInstruction(
      {9,   0,   0,   0,   161, 51,  89,  91,  115, 210, 217, 212, 76,  159,
       171, 200, 40,  150, 157, 70,  197, 71,  24,  44,  209, 108, 143, 4,
       58,  251, 215, 62,  201, 172, 159, 197, 9,   0,   0,   0,   0,   0,
       0,   0,   84,  69,  83,  84,  32,  83,  69,  69,  68,  16,  39,  0,
       0,   0,   0,   0,   0,   6,   221, 246, 225, 215, 101, 161, 147, 217,
       203, 225, 70,  206, 235, 121, 172, 28,  180, 133, 237, 95,  91,  55,
       145, 58,  140, 245, 133, 126, 255, 0,   169},
      mojom::SolanaSystemInstruction::kAllocateWithSeed,
      {{"base", kPubkey1},
       {"seed", "TEST SEED"},
       {"space", "10000"},
       {"owner_program", mojom::kSolanaTokenProgramId}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_SystemAssignWithSeed) {
  TestSystemInstruction(
      {10,  0,   0,   0,   161, 51,  89,  91,  115, 210, 217, 212, 76,
       159, 171, 200, 40,  150, 157, 70,  197, 71,  24,  44,  209, 108,
       143, 4,   58,  251, 215, 62,  201, 172, 159, 197, 9,   0,   0,
       0,   0,   0,   0,   0,   84,  69,  83,  84,  32,  83,  69,  69,
       68,  6,   221, 246, 225, 215, 101, 161, 147, 217, 203, 225, 70,
       206, 235, 121, 172, 28,  180, 133, 237, 95,  91,  55,  145, 58,
       140, 245, 133, 126, 255, 0,   169},
      mojom::SolanaSystemInstruction::kAssignWithSeed,
      {{"base", kPubkey1},
       {"seed", "TEST SEED"},
       {"owner_program", mojom::kSolanaTokenProgramId}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_SystemTransferWithSeed) {
  TestSystemInstruction(
      {11,  0,  0,   0,   160, 134, 1,   0,   0,   0,   0,   0,   9,
       0,   0,  0,   0,   0,   0,   0,   84,  69,  83,  84,  32,  83,
       69,  69, 68,  6,   221, 246, 225, 215, 101, 161, 147, 217, 203,
       225, 70, 206, 235, 121, 172, 28,  180, 133, 237, 95,  91,  55,
       145, 58, 140, 245, 133, 126, 255, 0,   169},
      mojom::SolanaSystemInstruction::kTransferWithSeed,
      {{"lamports", "100000"},
       {"from_seed", "TEST SEED"},
       {"from_owner_program", mojom::kSolanaTokenProgramId}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenInitializeMint) {
  TestTokenInstruction(
      {0,   9,   161, 51,  89,  91,  115, 210, 217, 212, 76,  159, 171, 200,
       40,  150, 157, 70,  197, 71,  24,  44,  209, 108, 143, 4,   58,  251,
       215, 62,  201, 172, 159, 197, 1,   179, 10,  45,  120, 165, 79,  23,
       213, 130, 206, 38,  194, 56,  107, 31,  15,  105, 52,  170, 204, 201,
       218, 15,  234, 163, 176, 140, 194, 226, 39,  121, 169},
      mojom::SolanaTokenInstruction::kInitializeMint,
      {{"decimals", "9"},
       {"mint_authority", kPubkey1},
       {"freeze_authority", kPubkey2}});

  // Without passing optional freeze authority.
  // Rust impl does not expect padded Pubkey(0) at the end.
  TestTokenInstruction(
      {0,   9,   161, 51,  89,  91, 115, 210, 217, 212, 76,  159,
       171, 200, 40,  150, 157, 70, 197, 71,  24,  44,  209, 108,
       143, 4,   58,  251, 215, 62, 201, 172, 159, 197, 0},
      mojom::SolanaTokenInstruction::kInitializeMint,
      {{"decimals", "9"}, {"mint_authority", kPubkey1}});

  // Typescript impl currently padded Pubkey(0) at the end.
  TestTokenInstruction(
      {0,   9,   161, 51,  89,  91,  115, 210, 217, 212, 76,  159, 171, 200,
       40,  150, 157, 70,  197, 71,  24,  44,  209, 108, 143, 4,   58,  251,
       215, 62,  201, 172, 159, 197, 0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
      mojom::SolanaTokenInstruction::kInitializeMint,
      {{"decimals", "9"}, {"mint_authority", kPubkey1}}, true);
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenInitializeAccount) {
  TestTokenInstruction({1}, mojom::SolanaTokenInstruction::kInitializeAccount,
                       {});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenInitializeMultisig) {
  TestTokenInstruction({2, 2},
                       mojom::SolanaTokenInstruction::kInitializeMultisig,
                       {{"num_of_signers", "2"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenTransfer) {
  TestTokenInstruction({3, 160, 134, 1, 0, 0, 0, 0, 0},
                       mojom::SolanaTokenInstruction::kTransfer,
                       {{"amount", "100000"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenApprove) {
  TestTokenInstruction({4, 160, 134, 1, 0, 0, 0, 0, 0},
                       mojom::SolanaTokenInstruction::kApprove,
                       {{"amount", "100000"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenRevoke) {
  TestTokenInstruction({5}, mojom::SolanaTokenInstruction::kRevoke, {});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenSetAuthority) {
  TestTokenInstruction(
      {6,   1,  1,   179, 10,  45,  120, 165, 79, 23,  213, 130,
       206, 38, 194, 56,  107, 31,  15,  105, 52, 170, 204, 201,
       218, 15, 234, 163, 176, 140, 194, 226, 39, 121, 169},
      mojom::SolanaTokenInstruction::kSetAuthority,
      {{"authority_type", "1"}, {"new_authority", kPubkey2}});

  // Without passing optional new authority.
  // Rust impl does not expect padded Pubkey(0) at the end.
  TestTokenInstruction({6, 1, 0}, mojom::SolanaTokenInstruction::kSetAuthority,
                       {{"authority_type", "1"}});

  // Typescript impl currently padded Pubkey(0) at the end.
  TestTokenInstruction({6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       mojom::SolanaTokenInstruction::kSetAuthority,
                       {{"authority_type", "1"}}, true);
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenMintTo) {
  TestTokenInstruction({7, 16, 39, 0, 0, 0, 0, 0, 0},
                       mojom::SolanaTokenInstruction::kMintTo,
                       {{"amount", "10000"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenBurn) {
  TestTokenInstruction({8, 232, 3, 0, 0, 0, 0, 0, 0},
                       mojom::SolanaTokenInstruction::kBurn,
                       {{"amount", "1000"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenCloseAccount) {
  TestTokenInstruction({9}, mojom::SolanaTokenInstruction::kCloseAccount, {});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenFreezeAccount) {
  TestTokenInstruction({10}, mojom::SolanaTokenInstruction::kFreezeAccount, {});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenThawAccount) {
  TestTokenInstruction({11}, mojom::SolanaTokenInstruction::kThawAccount, {});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TransferChecked) {
  TestTokenInstruction({12, 100, 0, 0, 0, 0, 0, 0, 0, 9},
                       mojom::SolanaTokenInstruction::kTransferChecked,
                       {{"amount", "100"}, {"decimals", "9"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_ApproveChecked) {
  TestTokenInstruction({13, 232, 3, 0, 0, 0, 0, 0, 0, 8},
                       mojom::SolanaTokenInstruction::kApproveChecked,
                       {{"amount", "1000"}, {"decimals", "8"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_MintToChecked) {
  TestTokenInstruction({14, 160, 134, 1, 0, 0, 0, 0, 0, 9},
                       mojom::SolanaTokenInstruction::kMintToChecked,
                       {{"amount", "100000"}, {"decimals", "9"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_BurnChecked) {
  TestTokenInstruction({15, 100, 0, 0, 0, 0, 0, 0, 0, 9},
                       mojom::SolanaTokenInstruction::kBurnChecked,
                       {{"amount", "100"}, {"decimals", "9"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_InitializeAccount2) {
  TestTokenInstruction({16,  179, 10,  45,  120, 165, 79,  23,  213, 130, 206,
                        38,  194, 56,  107, 31,  15,  105, 52,  170, 204, 201,
                        218, 15,  234, 163, 176, 140, 194, 226, 39,  121, 169},
                       mojom::SolanaTokenInstruction::kInitializeAccount2,
                       {{"owner", kPubkey2}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_TokenSyncNative) {
  TestTokenInstruction({17}, mojom::SolanaTokenInstruction::kSyncNative, {});
}

TEST_F(SolanaInstructionDecoderTest, Decode_InitializeAccount3) {
  TestTokenInstruction({18,  179, 10,  45,  120, 165, 79,  23,  213, 130, 206,
                        38,  194, 56,  107, 31,  15,  105, 52,  170, 204, 201,
                        218, 15,  234, 163, 176, 140, 194, 226, 39,  121, 169},
                       mojom::SolanaTokenInstruction::kInitializeAccount3,
                       {{"owner", kPubkey2}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_InitializeMultisig2) {
  TestTokenInstruction({19, 2},
                       mojom::SolanaTokenInstruction::kInitializeMultisig2,
                       {{"num_of_signers", "2"}});
}

TEST_F(SolanaInstructionDecoderTest, Decode_InitializeMint2) {
  TestTokenInstruction(
      {20,  9,   161, 51,  89,  91,  115, 210, 217, 212, 76,  159, 171, 200,
       40,  150, 157, 70,  197, 71,  24,  44,  209, 108, 143, 4,   58,  251,
       215, 62,  201, 172, 159, 197, 1,   179, 10,  45,  120, 165, 79,  23,
       213, 130, 206, 38,  194, 56,  107, 31,  15,  105, 52,  170, 204, 201,
       218, 15,  234, 163, 176, 140, 194, 226, 39,  121, 169},
      mojom::SolanaTokenInstruction::kInitializeMint2,
      {{"decimals", "9"},
       {"mint_authority", kPubkey1},
       {"freeze_authority", kPubkey2}});

  // Without passing optional freeze authority.
  // Rust impl does not expect padded Pubkey(0) at the end.
  TestTokenInstruction(
      {20,  9,   161, 51,  89,  91, 115, 210, 217, 212, 76,  159,
       171, 200, 40,  150, 157, 70, 197, 71,  24,  44,  209, 108,
       143, 4,   58,  251, 215, 62, 201, 172, 159, 197, 0},
      mojom::SolanaTokenInstruction::kInitializeMint2,
      {{"decimals", "9"}, {"mint_authority", kPubkey1}});

  // Typescript impl currently padded Pubkey(0) at the end.
  TestTokenInstruction(
      {20,  9,   161, 51,  89,  91,  115, 210, 217, 212, 76,  159, 171, 200,
       40,  150, 157, 70,  197, 71,  24,  44,  209, 108, 143, 4,   58,  251,
       215, 62,  201, 172, 159, 197, 0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
      mojom::SolanaTokenInstruction::kInitializeMint2,
      {{"decimals", "9"}, {"mint_authority", kPubkey1}}, true);
}

}  // namespace brave_wallet::solana_ins_data_decoder
