/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/test/gtest_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_compiled_instruction.h"
#include "brave/components/brave_wallet/browser/solana_instruction_data_decoder.h"
#include "brave/components/brave_wallet/browser/solana_message_address_table_lookup.h"
#include "brave/components/brave_wallet/browser/solana_message_header.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/solana_address.h"

namespace {

constexpr char kAccount1[] = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
constexpr char kAccount2[] = "83astBRguLMdt2h5U1Tpdq5tjFoJ6noeGwaY3mDLVcri";
constexpr char kAccount3[] = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
constexpr char kAccount4[] = "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD";
constexpr char kAccount5[] = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";

}  // namespace

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SolanaInstructionUnitTest, FromToCompiledInstruction) {
  SolanaInstruction expected_ins(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kAccount1, std::nullopt, true, false),
       SolanaAccountMeta(kAccount2, 1, false, true),
       SolanaAccountMeta(kAccount4, 3, false, true),
       SolanaAccountMeta(kAccount3, 1, false, false)},
      {});

  // static accounts: signer1, program Id
  // dynamic write: 2, 0, 1
  // dynamic read: 0, 1, 0
  SolanaMessageAddressTableLookup lookup1(*SolanaAddress::FromBase58(kAccount2),
                                          {1, 8}, {});
  SolanaMessageAddressTableLookup lookup2(*SolanaAddress::FromBase58(kAccount3),
                                          {}, {1});
  SolanaMessageAddressTableLookup lookup3(*SolanaAddress::FromBase58(kAccount4),
                                          {3}, {});
  std::vector<SolanaMessageAddressTableLookup> lookups;
  lookups.push_back(std::move(lookup1));
  lookups.push_back(std::move(lookup2));
  lookups.push_back(std::move(lookup3));

  // {kAccount1, system_program_id, kAccount2_write_index_0,
  //  kAccount2_write_index_1, kAccount4_write_index_0,
  //  kAccount2_read_index_0}
  // account_indexes: kAccount1, kAccount2_write_index_0,
  //                  kAccount4_write_index_0, kAccount3_read_index_0
  SolanaCompiledInstruction compiled_ins(1, {0, 2, 4, 5}, {});
  std::vector<SolanaAddress> static_accounts(
      {*SolanaAddress::FromBase58(kAccount1),
       *SolanaAddress::FromBase58(mojom::kSolanaSystemProgramId)});

  SolanaMessageHeader message_header(1, 1, 1);

  auto ins = SolanaInstruction::FromCompiledInstruction(
      compiled_ins, message_header, static_accounts, lookups, 3, 1);
  EXPECT_EQ(*ins, expected_ins);

  EXPECT_EQ(ins->GetProgramId(), expected_ins.GetProgramId());
  EXPECT_EQ(ins->GetAccounts(), expected_ins.GetAccounts());

  auto compiled_ins_from_ins = SolanaCompiledInstruction::FromInstruction(
      *ins, static_accounts, lookups, 3);
  ASSERT_TRUE(compiled_ins_from_ins);
  EXPECT_EQ(compiled_ins, *compiled_ins_from_ins);

  // Test program ID index is out of bound.
  auto invalid_ins = SolanaCompiledInstruction(6, {0, 2, 4, 5}, {});
  EXPECT_FALSE(SolanaInstruction::FromCompiledInstruction(
      invalid_ins, message_header, static_accounts, lookups, 3, 1));

  // Test account index is out of bound. (static account)
  invalid_ins = SolanaCompiledInstruction(1, {6, 2, 4, 5}, {});
  EXPECT_FALSE(SolanaInstruction::FromCompiledInstruction(
      invalid_ins, message_header, static_accounts, lookups, 3, 1));

  // Test account index is out of bound. (dynamic account)
  invalid_ins = SolanaCompiledInstruction(1, {0, 2, 6, 5}, {});
  EXPECT_FALSE(SolanaInstruction::FromCompiledInstruction(
      invalid_ins, message_header, static_accounts, lookups, 3, 1));

  // Test all possible is_signer and is_writable values.
  SolanaInstruction expected_ins2(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kAccount1, std::nullopt, true, true),
       SolanaAccountMeta(kAccount2, std::nullopt, true, false),
       SolanaAccountMeta(kAccount3, std::nullopt, false, true),
       SolanaAccountMeta(kAccount4, std::nullopt, false, false),
       SolanaAccountMeta(kAccount5, 2, false, true),
       SolanaAccountMeta(kAccount5, 6, false, false)},
      {});

  SolanaMessageAddressTableLookup lookup4(*SolanaAddress::FromBase58(kAccount5),
                                          {1, 2, 3}, {4, 5, 6});
  std::vector<SolanaMessageAddressTableLookup> lookups2;
  lookups2.push_back(std::move(lookup4));

  // Combined array for account indexing:
  // {kAccount1, kAccount2, kAccount3, system_program_id, kAccount4,
  //  kAccount5_write_index_0, kAccount5_write_index_1, kAccount5_write_index_2,
  //  kAccount5_read_index_0, kAccount5_read_index_1, kAccount5_read_index_2}
  SolanaCompiledInstruction compiled_ins2(3, {0, 1, 2, 4, 6, 10}, {});
  std::vector<SolanaAddress> static_accounts2(
      {*SolanaAddress::FromBase58(kAccount1),
       *SolanaAddress::FromBase58(kAccount2),
       *SolanaAddress::FromBase58(kAccount3),
       *SolanaAddress::FromBase58(mojom::kSolanaSystemProgramId),
       *SolanaAddress::FromBase58(kAccount4)});

  SolanaMessageHeader message_header2(2, 1, 1);

  auto ins2 = SolanaInstruction::FromCompiledInstruction(
      compiled_ins2, message_header2, static_accounts2, lookups2, 3, 3);
  EXPECT_EQ(*ins2, expected_ins2);

  EXPECT_EQ(ins2->GetProgramId(), expected_ins2.GetProgramId());
  EXPECT_EQ(ins2->GetAccounts(), expected_ins2.GetAccounts());

  auto compiled_ins_from_ins2 = SolanaCompiledInstruction::FromInstruction(
      *ins2, static_accounts2, lookups2, 3);
  ASSERT_TRUE(compiled_ins_from_ins2);
  EXPECT_EQ(compiled_ins2, *compiled_ins_from_ins2);
}

TEST(SolanaInstructionUnitTest, FromToValue) {
  std::string from_account = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
  std::string to_account = from_account;
  std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  SolanaInstruction instruction(
      // Program ID
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, std::nullopt, true, true),
       SolanaAccountMeta(to_account, std::nullopt, false, true)},
      data);

  base::Value::Dict value = instruction.ToValue();
  auto expect_instruction_value = base::test::ParseJson(R"(
    {
      "program_id": "11111111111111111111111111111111",
      "accounts": [
        {
          "pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
          "is_signer": true,
          "is_writable": true
        },
        {
          "pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
          "is_signer": false,
          "is_writable": true
        }
      ],
      "data": "AgAAAICWmAAAAAAA",
      "decoded_data": {
        "account_params": [
          {
            "name": "from_account",
            "localized_name": "From Account",
          },
          {
            "name": "to_account",
            "localized_name": "To Account"
          }
        ],
        "params": [
          {
            "name": "lamports",
            "localized_name": "Lamports",
            "value": "10000000",
            "type": 2
          }
        ],
        "sys_ins_type": "2"
      }
    }
  )");
  EXPECT_EQ(value, expect_instruction_value.GetDict());

  auto instruction_from_value = SolanaInstruction::FromValue(value);
  EXPECT_EQ(instruction, instruction_from_value);

  std::vector<std::string> invalid_value_strings = {
      "{}", R"({"program_id": "program id", "accounts": []})",
      R"({"program_id": "program id", "data": ""})",
      R"({"accounts": [], "data": ""})"};

  for (const auto& invalid_value_string : invalid_value_strings) {
    auto invalid_value = base::test::ParseJson(invalid_value_string);
    EXPECT_FALSE(SolanaInstruction::FromValue(invalid_value.GetDict()))
        << ":" << invalid_value_string;
  }
}

TEST(SolanaInstructionUnitTest, FromMojomSolanaInstructions) {
  const std::string pubkey1 = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
  const std::string pubkey2 = "83astBRguLMdt2h5U1Tpdq5tjFoJ6noeGwaY3mDLVcri";
  std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};
  mojom::SolanaAccountMetaPtr mojom_account_meta1 =
      mojom::SolanaAccountMeta::New(pubkey1, nullptr, true, false);
  mojom::SolanaAccountMetaPtr mojom_account_meta2 =
      mojom::SolanaAccountMeta::New(pubkey2, nullptr, false, true);
  std::vector<mojom::SolanaAccountMetaPtr> mojom_account_metas1;
  mojom_account_metas1.push_back(mojom_account_meta1.Clone());
  mojom_account_metas1.push_back(mojom_account_meta2.Clone());
  std::vector<mojom::SolanaAccountMetaPtr> mojom_account_metas2;
  mojom_account_metas2.push_back(mojom_account_meta2.Clone());
  mojom_account_metas2.push_back(mojom_account_meta1.Clone());

  std::vector<mojom::SolanaInstructionParamPtr> mojom_params;
  mojom_params.emplace_back(mojom::SolanaInstructionParam::New(
      "lamports", "Lamports", "10000000",
      mojom::SolanaInstructionParamType::kUint64));
  auto mojom_decoded_data = mojom::DecodedSolanaInstructionData::New(
      static_cast<uint32_t>(mojom::SolanaSystemInstruction::kTransfer),
      solana_ins_data_decoder::GetMojomAccountParamsForTesting(
          mojom::SolanaSystemInstruction::kTransfer, std::nullopt),
      std::move(mojom_params));

  const std::string config_program =
      "Config1111111111111111111111111111111111111";
  mojom::SolanaInstructionPtr mojom_instruction1 =
      mojom::SolanaInstruction::New(mojom::kSolanaSystemProgramId,
                                    std::move(mojom_account_metas1), data,
                                    std::move(mojom_decoded_data));
  mojom::SolanaInstructionPtr mojom_instruction2 =
      mojom::SolanaInstruction::New(
          config_program, std::move(mojom_account_metas2), data, nullptr);
  std::vector<mojom::SolanaInstructionPtr> mojom_instructions;
  mojom_instructions.push_back(std::move(mojom_instruction1));
  mojom_instructions.push_back(std::move(mojom_instruction2));

  std::vector<SolanaInstruction> instructions;
  SolanaInstruction::FromMojomSolanaInstructions(mojom_instructions,
                                                 &instructions);
  EXPECT_EQ(std::vector<SolanaInstruction>(
                {SolanaInstruction(
                     mojom::kSolanaSystemProgramId,
                     {SolanaAccountMeta(pubkey1, std::nullopt, true, false),
                      SolanaAccountMeta(pubkey2, std::nullopt, false, true)},
                     data),
                 SolanaInstruction(
                     config_program,
                     {SolanaAccountMeta(pubkey2, std::nullopt, false, true),
                      SolanaAccountMeta(pubkey1, std::nullopt, true, false)},
                     data, std::nullopt)}),
            instructions);
}

TEST(SolanaInstructionUnitTest, ToMojomSolanaInstruction) {
  const std::string pubkey1 = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
  const std::string pubkey2 = "83astBRguLMdt2h5U1Tpdq5tjFoJ6noeGwaY3mDLVcri";
  std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  SolanaInstruction instruction(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(pubkey1, std::nullopt, true, false),
       SolanaAccountMeta(pubkey2, std::nullopt, false, true)},
      data);

  auto mojom_instruction = instruction.ToMojomSolanaInstruction();
  ASSERT_TRUE(mojom_instruction);
  EXPECT_EQ(mojom_instruction->program_id, mojom::kSolanaSystemProgramId);
  EXPECT_EQ(mojom_instruction->account_metas.size(), 2u);
  EXPECT_EQ(mojom_instruction->account_metas[0],
            mojom::SolanaAccountMeta::New(pubkey1, nullptr, true, false));
  EXPECT_EQ(mojom_instruction->account_metas[1],
            mojom::SolanaAccountMeta::New(pubkey2, nullptr, false, true));
  EXPECT_EQ(mojom_instruction->data, data);
  std::vector<mojom::SolanaInstructionParamPtr> mojom_params;
  mojom_params.emplace_back(mojom::SolanaInstructionParam::New(
      "lamports", "Lamports", "10000000",
      mojom::SolanaInstructionParamType::kUint64));
  EXPECT_EQ(
      mojom_instruction->decoded_data,
      mojom::DecodedSolanaInstructionData::New(
          static_cast<uint32_t>(mojom::SolanaSystemInstruction::kTransfer),
          solana_ins_data_decoder::GetMojomAccountParamsForTesting(
              mojom::SolanaSystemInstruction::kTransfer, std::nullopt),
          std::move(mojom_params)));
}

}  // namespace brave_wallet
