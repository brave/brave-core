/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_data_decoder.h"

#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/browser/solana_instruction_builder.h"
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

TEST_F(SolanaInstructionDecoderTest, DecodeSystemCreateAccount) {
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

TEST_F(SolanaInstructionDecoderTest, DecodeSystemAssign) {
  TestSystemInstruction(
      {1,   0,   0,   0,   6,   221, 246, 225, 215, 101, 161, 147,
       217, 203, 225, 70,  206, 235, 121, 172, 28,  180, 133, 237,
       95,  91,  55,  145, 58,  140, 245, 133, 126, 255, 0,   169},
      mojom::SolanaSystemInstruction::kAssign,
      {{"owner_program", mojom::kSolanaTokenProgramId}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeSystemTransfer) {
  TestSystemInstruction({2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0},
                        mojom::SolanaSystemInstruction::kTransfer,
                        {{"lamports", "10000000"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeSystemCreateAccountWithSeed) {
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

TEST_F(SolanaInstructionDecoderTest, DecodeSystemAdvanceNonceAccount) {
  TestSystemInstruction(
      {4, 0, 0, 0}, mojom::SolanaSystemInstruction::kAdvanceNonceAccount, {});
}

TEST_F(SolanaInstructionDecoderTest, DecodeSystemWithdrawNonceAccount) {
  TestSystemInstruction({5, 0, 0, 0, 16, 39, 0, 0, 0, 0, 0, 0},
                        mojom::SolanaSystemInstruction::kWithdrawNonceAccount,
                        {{"lamports", "10000"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeSystemInitializeNonceAccount) {
  TestSystemInstruction(
      {6,   0,   0,   0,   161, 51,  89,  91, 115, 210, 217, 212,
       76,  159, 171, 200, 40,  150, 157, 70, 197, 71,  24,  44,
       209, 108, 143, 4,   58,  251, 215, 62, 201, 172, 159, 197},
      mojom::SolanaSystemInstruction::kInitializeNonceAccount,
      {{"authority", kPubkey1}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeSystemAuthorizeNonceAccount) {
  TestSystemInstruction(
      {7,   0,   0,  0,   179, 10,  45,  120, 165, 79, 23,  213,
       130, 206, 38, 194, 56,  107, 31,  15,  105, 52, 170, 204,
       201, 218, 15, 234, 163, 176, 140, 194, 226, 39, 121, 169},
      mojom::SolanaSystemInstruction::kAuthorizeNonceAccount,
      {{"new_authority", kPubkey2}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeSystemAllocate) {
  TestSystemInstruction({8, 0, 0, 0, 16, 39, 0, 0, 0, 0, 0, 0},
                        mojom::SolanaSystemInstruction::kAllocate,
                        {{"space", "10000"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeSystemAllocateWithSeed) {
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

TEST_F(SolanaInstructionDecoderTest, DecodeSystemAssignWithSeed) {
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

TEST_F(SolanaInstructionDecoderTest, DecodeSystemTransferWithSeed) {
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

TEST_F(SolanaInstructionDecoderTest, DecodeTokenInitializeMint) {
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

TEST_F(SolanaInstructionDecoderTest, DecodeTokenInitializeAccount) {
  TestTokenInstruction({1}, mojom::SolanaTokenInstruction::kInitializeAccount,
                       {});
}

TEST_F(SolanaInstructionDecoderTest, DecodeTokenInitializeMultisig) {
  TestTokenInstruction({2, 2},
                       mojom::SolanaTokenInstruction::kInitializeMultisig,
                       {{"num_of_signers", "2"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeTokenTransfer) {
  TestTokenInstruction({3, 160, 134, 1, 0, 0, 0, 0, 0},
                       mojom::SolanaTokenInstruction::kTransfer,
                       {{"amount", "100000"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeTokenApprove) {
  TestTokenInstruction({4, 160, 134, 1, 0, 0, 0, 0, 0},
                       mojom::SolanaTokenInstruction::kApprove,
                       {{"amount", "100000"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeTokenRevoke) {
  TestTokenInstruction({5}, mojom::SolanaTokenInstruction::kRevoke, {});
}

TEST_F(SolanaInstructionDecoderTest, DecodeTokenSetAuthority) {
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

TEST_F(SolanaInstructionDecoderTest, DecodeTokenMintTo) {
  TestTokenInstruction({7, 16, 39, 0, 0, 0, 0, 0, 0},
                       mojom::SolanaTokenInstruction::kMintTo,
                       {{"amount", "10000"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeTokenBurn) {
  TestTokenInstruction({8, 232, 3, 0, 0, 0, 0, 0, 0},
                       mojom::SolanaTokenInstruction::kBurn,
                       {{"amount", "1000"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeTokenCloseAccount) {
  TestTokenInstruction({9}, mojom::SolanaTokenInstruction::kCloseAccount, {});
}

TEST_F(SolanaInstructionDecoderTest, DecodeTokenFreezeAccount) {
  TestTokenInstruction({10}, mojom::SolanaTokenInstruction::kFreezeAccount, {});
}

TEST_F(SolanaInstructionDecoderTest, DecodeTokenThawAccount) {
  TestTokenInstruction({11}, mojom::SolanaTokenInstruction::kThawAccount, {});
}

TEST_F(SolanaInstructionDecoderTest, DecodeTransferChecked) {
  TestTokenInstruction({12, 100, 0, 0, 0, 0, 0, 0, 0, 9},
                       mojom::SolanaTokenInstruction::kTransferChecked,
                       {{"amount", "100"}, {"decimals", "9"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeApproveChecked) {
  TestTokenInstruction({13, 232, 3, 0, 0, 0, 0, 0, 0, 8},
                       mojom::SolanaTokenInstruction::kApproveChecked,
                       {{"amount", "1000"}, {"decimals", "8"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeMintToChecked) {
  TestTokenInstruction({14, 160, 134, 1, 0, 0, 0, 0, 0, 9},
                       mojom::SolanaTokenInstruction::kMintToChecked,
                       {{"amount", "100000"}, {"decimals", "9"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeBurnChecked) {
  TestTokenInstruction({15, 100, 0, 0, 0, 0, 0, 0, 0, 9},
                       mojom::SolanaTokenInstruction::kBurnChecked,
                       {{"amount", "100"}, {"decimals", "9"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeInitializeAccount2) {
  TestTokenInstruction({16,  179, 10,  45,  120, 165, 79,  23,  213, 130, 206,
                        38,  194, 56,  107, 31,  15,  105, 52,  170, 204, 201,
                        218, 15,  234, 163, 176, 140, 194, 226, 39,  121, 169},
                       mojom::SolanaTokenInstruction::kInitializeAccount2,
                       {{"owner", kPubkey2}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeTokenSyncNative) {
  TestTokenInstruction({17}, mojom::SolanaTokenInstruction::kSyncNative, {});
}

TEST_F(SolanaInstructionDecoderTest, DecodeInitializeAccount3) {
  TestTokenInstruction({18,  179, 10,  45,  120, 165, 79,  23,  213, 130, 206,
                        38,  194, 56,  107, 31,  15,  105, 52,  170, 204, 201,
                        218, 15,  234, 163, 176, 140, 194, 226, 39,  121, 169},
                       mojom::SolanaTokenInstruction::kInitializeAccount3,
                       {{"owner", kPubkey2}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeInitializeMultisig2) {
  TestTokenInstruction({19, 2},
                       mojom::SolanaTokenInstruction::kInitializeMultisig2,
                       {{"num_of_signers", "2"}});
}

TEST_F(SolanaInstructionDecoderTest, DecodeInitializeMint2) {
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

TEST_F(SolanaInstructionDecoderTest, GetComputeBudgetInstructionType) {
  // Recognizes compute limit instruction
  auto instruction = solana::compute_budget_program::SetComputeUnitLimit(0);
  auto instruction_type = GetComputeBudgetInstructionType(
      instruction.data(), instruction.GetProgramId());
  ASSERT_TRUE(instruction_type);
  EXPECT_EQ(*instruction_type,
            mojom::SolanaComputeBudgetInstruction::kSetComputeUnitLimit);

  // Recognizes compute unit instruction
  instruction = solana::compute_budget_program::SetComputeUnitPrice(0);
  instruction_type = GetComputeBudgetInstructionType(
      instruction.data(), instruction.GetProgramId());
  ASSERT_TRUE(instruction_type);
  EXPECT_EQ(*instruction_type,
            mojom::SolanaComputeBudgetInstruction::kSetComputeUnitPrice);

  // Returns nullopt for system instruction
  std::string from_pubkey = "5PfVQ7u360kP2uvPX9oeKtp3bpHbSYKJyQEEMKB3eF7V";
  std::string to_pubkey = "H5sGH5Avk14e6h6gV8vXuBwYxVtT1VkmmVU5sPMinHho";
  auto system_instruction =
      solana::system_program::Transfer(from_pubkey, to_pubkey, 1000);
  ASSERT_TRUE(system_instruction);
  instruction_type = GetComputeBudgetInstructionType(
      (*system_instruction).data(), (*system_instruction).GetProgramId());
  ASSERT_FALSE(instruction_type);

  // Returns nullopt for empty data
  instruction_type =
      GetComputeBudgetInstructionType({}, mojom::kSolanaComputeBudgetProgramId);
  ASSERT_FALSE(instruction_type);

  // Returns nullopt for invalid instruction type
  std::vector<uint8_t> invalid_data = {255};  // 255 is not a valid enum value
  instruction_type = GetComputeBudgetInstructionType(
      invalid_data, mojom::kSolanaComputeBudgetProgramId);
  ASSERT_FALSE(instruction_type);
}

TEST_F(SolanaInstructionDecoderTest, IsCompressedNftTransferInstruction) {
  // Empty data is not a compressed NFT transfer instruction.
  EXPECT_FALSE(
      IsCompressedNftTransferInstruction({}, mojom::kSolanaBubbleGumProgramId));

  // Compressed NFT transfer instruction is recognized.
  std::vector<uint8_t> data = {
      // Contains compressed NFT transfer disctiminator.
      0xa3, 0x34, 0xc8, 0xe7, 0x8c, 0x03, 0x45, 0xba, 0x44, 0x3f, 0xca, 0x38,
      0xd1, 0x3e, 0x68, 0xf2, 0x95, 0xaf, 0xfc, 0x5f, 0x34, 0x31, 0xf3, 0x75,
      0xba, 0xd8, 0xd3, 0x82, 0x90, 0x1a, 0x94, 0x7f, 0x72, 0x96, 0xfc, 0xd8,
      0x79, 0x8a, 0xb7, 0x98, 0x3b, 0x17, 0x52, 0x74, 0x15, 0x6f, 0x94, 0x1a,
      0xe6, 0xc6, 0x1e, 0x0e, 0xb4, 0x6c, 0xcf, 0x64, 0xd6, 0x8f, 0xfd, 0x34,
      0xb7, 0x68, 0x6d, 0x97, 0x32, 0x45, 0x7e, 0x8a, 0x5c, 0x1a, 0x80, 0x31,
      0x9b, 0x22, 0x99, 0xb4, 0xc2, 0x20, 0x0e, 0x5e, 0xef, 0x2e, 0x12, 0xb1,
      0x6d, 0x4f, 0xbd, 0xf1, 0x2e, 0x11, 0xe1, 0x4f, 0xb2, 0x76, 0xc3, 0x91,
      0x21, 0x88, 0x34, 0xf3, 0x0a, 0xec, 0x39, 0x45, 0xa5, 0x15, 0x14, 0x00,
      0x00, 0x00, 0x00, 0x00, 0xa5, 0x15, 0x14, 0x00};
  EXPECT_TRUE(IsCompressedNftTransferInstruction(
      data, mojom::kSolanaBubbleGumProgramId));

  // Compressed NFT transfer instruction is not recognized for different program
  // id.
  EXPECT_FALSE(
      IsCompressedNftTransferInstruction(data, mojom::kSolanaTokenProgramId));
}

}  // namespace brave_wallet::solana_ins_data_decoder
