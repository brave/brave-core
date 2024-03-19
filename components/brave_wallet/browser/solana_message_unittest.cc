/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_message.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/sys_byteorder.h"
#include "base/test/gtest_util.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr char kFromAccount[] = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
constexpr char kToAccount[] = "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD";
constexpr char kTestAccount[] = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
constexpr char kRecentBlockhash[] =
    "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
constexpr uint64_t kLastValidBlockHeight = 3090;

SolanaMessage GetTestLegacyMessage() {
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};
  SolanaInstruction instruction(
      // Program ID
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, false, true)},
      data);
  auto message = SolanaMessage::CreateLegacyMessage(
      kRecentBlockhash, kLastValidBlockHeight, kFromAccount, {instruction});
  return std::move(*message);
}

SolanaMessage GetTestV0Message() {
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};
  SolanaMessageAddressTableLookup lookup(*SolanaAddress::FromBase58(kToAccount),
                                         {3, 1}, {2, 4});
  SolanaInstruction instruction(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, 1, false, true)},
      data);
  std::vector<SolanaMessageAddressTableLookup> lookups;
  lookups.push_back(std::move(lookup));
  return SolanaMessage(
      mojom::SolanaMessageVersion::kV0, kRecentBlockhash, kLastValidBlockHeight,
      kFromAccount, SolanaMessageHeader(1, 0, 1),
      {*SolanaAddress::FromBase58(kFromAccount),
       *SolanaAddress::FromBase58(mojom::kSolanaSystemProgramId)},
      {instruction}, std::move(lookups));
}

}  // namespace

TEST(SolanaMessageUnitTest, SerializeDeserialize) {
  std::vector<uint8_t> expected_bytes_legacy = {
      // Message header
      1,  // num_required_signatures
      0,  // num_readonly_signed_accounts
      1,  // num_readonly_unsigned_accounts

      // Account addresses compact array
      3,  // account addresses array length
      // account_addresses[0]: base58-decoded from account
      34, 208, 53, 54, 75, 46, 112, 55, 123, 15, 232, 9, 45, 178, 252, 196, 62,
      64, 169, 213, 66, 87, 192, 16, 152, 108, 254, 148, 183, 39, 51, 192,
      // account_addresses[1]: base58-decoded to account
      35, 209, 53, 54, 75, 46, 112, 55, 123, 15, 232, 9, 45, 178, 252, 196, 62,
      64, 169, 213, 66, 87, 192, 16, 152, 108, 254, 148, 183, 39, 51, 192,
      // account_addresses[2]: base58-decoded program ID in the instruction
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0,

      // Recent blockhash, base58-decoded
      131, 191, 83, 201, 108, 193, 222, 255, 176, 67, 136, 209, 219, 42, 6, 169,
      240, 137, 142, 185, 169, 6, 17, 87, 123, 6, 42, 55, 162, 64, 120, 91,

      // Instructions compact array
      1,                                        // instructions array length
      2,                                        // program id index
      2,                                        // length of accounts
      0, 1,                                     // account indices
      12,                                       // data length
      2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0  // data
  };

  std::vector<uint8_t> expected_bytes_v0 = {
      0x80,  // v0
      // Message header
      1,  // num_required_signatures
      0,  // num_readonly_signed_accounts
      1,  // num_readonly_unsigned_accounts

      // Account addresses compact array
      2,  // account addresses array length
      // account_addresses[0]: base58-decoded from account
      34, 208, 53, 54, 75, 46, 112, 55, 123, 15, 232, 9, 45, 178, 252, 196, 62,
      64, 169, 213, 66, 87, 192, 16, 152, 108, 254, 148, 183, 39, 51, 192,
      // account_addresses[2]: base58-decoded program ID in the instruction
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0,

      // Recent blockhash, base58-decoded
      131, 191, 83, 201, 108, 193, 222, 255, 176, 67, 136, 209, 219, 42, 6, 169,
      240, 137, 142, 185, 169, 6, 17, 87, 123, 6, 42, 55, 162, 64, 120, 91,

      // Instructions compact array
      1,                                         // instructions array length
      1,                                         // program id index
      2,                                         // length of accounts
      0, 3,                                      // account indices
      12,                                        // data length
      2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0,  // data

      // Address table lookups compact array.
      1,  // address table lookups array length
      // Pubkey of address table lookup (base58-decoded to account).
      35, 209, 53, 54, 75, 46, 112, 55, 123, 15, 232, 9, 45, 178, 252, 196, 62,
      64, 169, 213, 66, 87, 192, 16, 152, 108, 254, 148, 183, 39, 51, 192,
      // write indexes compact array
      2, 3, 1,
      // read indexes compact array
      2, 2, 4};

  auto message1 = GetTestLegacyMessage();
  auto message2 = GetTestV0Message();
  std::vector<std::pair<const SolanaMessage&, const std::vector<uint8_t>&>>
      test_pairs = {{message1, expected_bytes_legacy},
                    {message2, expected_bytes_v0}};

  for (const auto& test_pair : test_pairs) {
    const auto& message = test_pair.first;
    const auto& expected_bytes = test_pair.second;
    SCOPED_TRACE(message.version());

    std::vector<std::string> signers;
    auto message_bytes = message.Serialize(&signers);
    ASSERT_TRUE(message_bytes);
    EXPECT_EQ(signers, std::vector<std::string>({kFromAccount}));
    EXPECT_EQ(message_bytes.value(), expected_bytes);

    // Deserialize and serialize message again should have the same byte array.
    auto deserialized_message = SolanaMessage::Deserialize(*message_bytes);
    ASSERT_TRUE(deserialized_message);
    signers.clear();
    auto serialized_message = deserialized_message->Serialize(&signers);
    ASSERT_TRUE(serialized_message);
    EXPECT_EQ(serialized_message, *message_bytes);
    EXPECT_EQ(signers, std::vector<std::string>({kFromAccount}));

    std::vector<uint8_t> msg_with_left_over_bytes(expected_bytes);
    msg_with_left_over_bytes.push_back(0);
    EXPECT_FALSE(SolanaMessage::Deserialize(msg_with_left_over_bytes));

    for (size_t i = 0; i < message_bytes->size(); ++i) {
      EXPECT_FALSE(SolanaMessage::Deserialize(std::vector<uint8_t>(
          message_bytes->begin(), message_bytes->begin() + i)));
      EXPECT_FALSE(SolanaMessage::Deserialize(std::vector<uint8_t>(
          message_bytes->begin() + i + 1, message_bytes->end())));
    }
  }

  // Try serializing a message without blockhash should fail.
  SolanaInstruction instruction(
      // Program ID
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, false, true)},
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  auto message_without_blockhash =
      SolanaMessage::CreateLegacyMessage("", 0, kFromAccount, {instruction});
  ASSERT_TRUE(message_without_blockhash);
  EXPECT_FALSE(message_without_blockhash->Serialize(nullptr));
}

TEST(SolanaMessageUnitTest, GetSignerAccountsFromSerializedMessageLegacy) {
  SolanaInstruction ins(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, true, true),
       SolanaAccountMeta(kTestAccount, std::nullopt, false, true)},
      {});
  auto msg = SolanaMessage::CreateLegacyMessage(kRecentBlockhash, 0,
                                                kFromAccount, {ins});
  ASSERT_TRUE(msg);
  std::vector<std::string> signers_from_serialize;
  auto serialized_message = msg->Serialize(&signers_from_serialize);
  ASSERT_TRUE(serialized_message);
  auto signers = SolanaMessage::GetSignerAccountsFromSerializedMessage(
      *serialized_message);
  EXPECT_EQ(*signers, std::vector<std::string>({kFromAccount, kToAccount}));
  EXPECT_EQ(*signers, signers_from_serialize);

  EXPECT_FALSE(SolanaMessage::GetSignerAccountsFromSerializedMessage({}));
  // Has account length 1 but no accounts to read.
  EXPECT_FALSE(
      SolanaMessage::GetSignerAccountsFromSerializedMessage({1, 1, 1, 1}));
}

TEST(SolanaMessageUnitTest, SerializeExceedNumOfAccountMaxSize) {
  auto ins = SolanaInstruction(mojom::kSolanaSystemProgramId, {}, {});
  std::vector<SolanaAddress> static_accounts;
  for (size_t i = 0; i <= UINT8_MAX; ++i) {
    static_accounts.emplace_back(*SolanaAddress::FromBase58(kFromAccount));
  }

  // Number of static accounts exceeds UINT8_MAX.
  auto msg =
      SolanaMessage(mojom::SolanaMessageVersion::kLegacy, kRecentBlockhash,
                    kLastValidBlockHeight, kFromAccount, SolanaMessageHeader(),
                    std::move(static_accounts), {ins}, {});
  EXPECT_FALSE(msg.Serialize(nullptr));

  // Number of write indexes exceeds UINT8_MAX.
  std::vector<SolanaMessageAddressTableLookup> lookups;
  std::vector<uint8_t> max_indexes(UINT8_MAX, 1);
  lookups.emplace_back(*SolanaAddress::FromBase58(kToAccount), max_indexes,
                       std::vector<uint8_t>());
  lookups.emplace_back(*SolanaAddress::FromBase58(kTestAccount),
                       std::vector<uint8_t>({2}), std::vector<uint8_t>());
  ASSERT_TRUE(static_accounts.empty());
  msg =
      SolanaMessage(mojom::SolanaMessageVersion::kV0, kRecentBlockhash,
                    kLastValidBlockHeight, kFromAccount, SolanaMessageHeader(),
                    std::move(static_accounts), {ins}, std::move(lookups));
  EXPECT_FALSE(msg.Serialize(nullptr));

  // Number of read indexes exceeds UINT8_MAX.
  ASSERT_TRUE(static_accounts.empty());
  ASSERT_TRUE(lookups.empty());
  lookups.emplace_back(*SolanaAddress::FromBase58(kToAccount),
                       std::vector<uint8_t>(), max_indexes);
  lookups.emplace_back(*SolanaAddress::FromBase58(kTestAccount),
                       std::vector<uint8_t>(), std::vector<uint8_t>({2}));
  msg =
      SolanaMessage(mojom::SolanaMessageVersion::kV0, kRecentBlockhash,
                    kLastValidBlockHeight, kFromAccount, SolanaMessageHeader(),
                    std::move(static_accounts), {ins}, std::move(lookups));
  EXPECT_FALSE(msg.Serialize(nullptr));

  // Number of entries in combined static_accounts, write_indexes, read_indexes
  // array exceeds UINT8_MAX.
  ASSERT_TRUE(static_accounts.empty());
  ASSERT_TRUE(lookups.empty());
  for (size_t i = 0; i < UINT8_MAX / 3; ++i) {
    static_accounts.emplace_back(*SolanaAddress::FromBase58(kFromAccount));
  }
  std::vector<uint8_t> one_third_of_max_indexes(UINT8_MAX / 3, 1);
  lookups.emplace_back(*SolanaAddress::FromBase58(kToAccount),
                       one_third_of_max_indexes, std::vector<uint8_t>({2}));
  lookups.emplace_back(*SolanaAddress::FromBase58(kTestAccount),
                       std::vector<uint8_t>(), one_third_of_max_indexes);
  msg =
      SolanaMessage(mojom::SolanaMessageVersion::kV0, kRecentBlockhash,
                    kLastValidBlockHeight, kFromAccount, SolanaMessageHeader(),
                    std::move(static_accounts), {ins}, std::move(lookups));
  EXPECT_FALSE(msg.Serialize(nullptr));
}

TEST(SolanaMessageUnitTest, GetUniqueAccountMetas) {
  std::vector<SolanaAccountMeta> unique_account_metas;

  std::string program1 = mojom::kSolanaSystemProgramId;
  std::string program2 = "Config1111111111111111111111111111111111111";
  std::string account1 = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
  std::string account2 = "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD";
  std::string account3 = "CiDwVBFgWV9E5MvXWoLgnEgn2hK7rJikbvfWavzAQz3";
  std::string account4 = "GcdayuLaLyrdmUu324nahyv33G5poQdLUEZ1nEytDeP";
  std::string account5 = "LX3EUdRUBUa3TbsYXLEUdj9J3prXkWXvLYSWyYyc2Jj";

  SolanaAccountMeta account1_fee_payer(account1, std::nullopt, true, true);
  SolanaAccountMeta program1_non_signer_readonly(program1, std::nullopt, false,
                                                 false);

  // Test instructions with duplicate pubkeys.
  SolanaAccountMeta account2_non_signer_readonly(account2, std::nullopt, false,
                                                 false);
  SolanaInstruction instruction1(program1, {account2_non_signer_readonly}, {});
  SolanaMessage::GetUniqueAccountMetas(account1, {instruction1, instruction1},
                                       &unique_account_metas);
  // fee payer at first, and no duplicate account pubkeys.
  std::vector<SolanaAccountMeta> expected_account_metas = {
      account1_fee_payer, program1_non_signer_readonly,
      account2_non_signer_readonly};
  EXPECT_EQ(unique_account_metas, expected_account_metas);

  // Test account order.
  // signer-read-write -> signer-readonly -> non-signer-read-write ->
  // non-signer-readonly
  SolanaAccountMeta account3_non_signer_read_write(account3, std::nullopt,
                                                   false, true);
  SolanaAccountMeta account4_signer_readonly(account4, std::nullopt, true,
                                             false);
  SolanaAccountMeta account5_signer_read_write(account5, std::nullopt, true,
                                               true);
  SolanaAccountMeta program2_non_signer_readonly(program2, std::nullopt, false,
                                                 false);
  SolanaInstruction instruction2(
      program1, {account3_non_signer_read_write, account4_signer_readonly}, {});
  SolanaInstruction instruction3(program2, {account5_signer_read_write}, {});
  SolanaMessage::GetUniqueAccountMetas(
      account1, {instruction1, instruction2, instruction3},
      &unique_account_metas);
  expected_account_metas = {
      account1_fee_payer,           account5_signer_read_write,
      account4_signer_readonly,     account3_non_signer_read_write,
      program1_non_signer_readonly, account2_non_signer_readonly,
      program2_non_signer_readonly};
  EXPECT_EQ(unique_account_metas, expected_account_metas);

  // Test writable being updated when readonly and read-write both present for
  // the same account pubkey.
  SolanaAccountMeta account2_signer_read_write(account2, std::nullopt, true,
                                               true);
  SolanaAccountMeta account3_non_signer_readonly(account3, std::nullopt, false,
                                                 false);
  SolanaAccountMeta account4_non_signer_read_write(account4, std::nullopt,
                                                   false, true);
  SolanaAccountMeta account4_signer_read_write(account4, std::nullopt, true,
                                               true);
  SolanaAccountMeta account5_signer_readonly(account5, std::nullopt, true,
                                             false);

  instruction1 = SolanaInstruction(
      program1,
      {account2_non_signer_readonly, account3_non_signer_readonly,
       account4_signer_readonly, account5_signer_readonly},
      {});
  instruction2 = SolanaInstruction(
      program1,
      {account4_non_signer_read_write, account5_signer_read_write,
       account3_non_signer_read_write, account2_signer_read_write},
      {});
  SolanaMessage::GetUniqueAccountMetas(account1, {instruction1, instruction2},
                                       &unique_account_metas);
  expected_account_metas = {account1_fee_payer,
                            account5_signer_read_write,
                            account2_signer_read_write,
                            account4_signer_read_write,
                            account3_non_signer_read_write,
                            program1_non_signer_readonly};
  EXPECT_EQ(unique_account_metas, expected_account_metas);

  EXPECT_DCHECK_DEATH(
      SolanaMessage::GetUniqueAccountMetas(account1, {instruction1}, nullptr));
}

TEST(SolanaMessageUnitTest, ToSolanaTxData) {
  // legacy
  SolanaMessage message = GetTestLegacyMessage();
  auto solana_tx_data = message.ToSolanaTxData();
  ASSERT_TRUE(solana_tx_data);

  EXPECT_EQ(solana_tx_data->recent_blockhash, kRecentBlockhash);
  EXPECT_EQ(solana_tx_data->last_valid_block_height, kLastValidBlockHeight);
  EXPECT_EQ(solana_tx_data->fee_payer, kFromAccount);

  ASSERT_EQ(solana_tx_data->instructions.size(), 1u);
  EXPECT_EQ(solana_tx_data->instructions[0]->program_id,
            mojom::kSolanaSystemProgramId);
  EXPECT_EQ(solana_tx_data->instructions[0]->data,
            std::vector<uint8_t>({2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0}));

  ASSERT_EQ(solana_tx_data->instructions[0]->account_metas.size(), 2u);
  EXPECT_EQ(solana_tx_data->instructions[0]->account_metas[0]->pubkey,
            kFromAccount);
  EXPECT_FALSE(solana_tx_data->instructions[0]
                   ->account_metas[0]
                   ->addr_table_lookup_index);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[0]->is_signer);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[0]->is_writable);
  EXPECT_EQ(solana_tx_data->instructions[0]->account_metas[1]->pubkey,
            kToAccount);
  EXPECT_FALSE(solana_tx_data->instructions[0]
                   ->account_metas[1]
                   ->addr_table_lookup_index);
  EXPECT_FALSE(solana_tx_data->instructions[0]->account_metas[1]->is_signer);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[1]->is_writable);

  EXPECT_EQ(mojom::SolanaMessageVersion::kLegacy, solana_tx_data->version);
  EXPECT_EQ(mojom::SolanaMessageHeader::New(1, 0, 1),
            solana_tx_data->message_header);
  EXPECT_EQ(solana_tx_data->static_account_keys,
            std::vector<std::string>(
                {kFromAccount, kToAccount, mojom::kSolanaSystemProgramId}));
  EXPECT_TRUE(solana_tx_data->address_table_lookups.empty());

  // v0
  auto expected_solana_tx_data_v0 = std::move(solana_tx_data);
  expected_solana_tx_data_v0->instructions[0]
      ->account_metas[1]
      ->addr_table_lookup_index = mojom::OptionalUint8::New(1);
  expected_solana_tx_data_v0->version = mojom::SolanaMessageVersion::kV0;
  expected_solana_tx_data_v0->static_account_keys = {
      kFromAccount, mojom::kSolanaSystemProgramId};
  std::vector<mojom::SolanaMessageAddressTableLookupPtr> mojom_lookups;
  auto mojom_lookup = mojom::SolanaMessageAddressTableLookup::New(
      std::string(kToAccount), std::vector<uint8_t>({3, 1}),
      std::vector<uint8_t>({2, 4}));
  mojom_lookups.push_back(std::move(mojom_lookup));
  expected_solana_tx_data_v0->address_table_lookups = std::move(mojom_lookups);

  SolanaMessage message2 = GetTestV0Message();
  solana_tx_data = message2.ToSolanaTxData();
  ASSERT_TRUE(solana_tx_data);

  EXPECT_EQ(solana_tx_data, expected_solana_tx_data_v0);
}

TEST(SolanaMessageUnitTest, FromToValue) {
  // legacy
  SolanaMessage message = GetTestLegacyMessage();
  base::Value::Dict value = message.ToValue();
  auto expect_message_value = base::JSONReader::Read(R"(
    {
      "version": 0,
      "recent_blockhash": "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6",
      "last_valid_block_height": "3090",
      "fee_payer": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
      "message_header": {
        "num_required_signatures": "1",
        "num_readonly_signed_accounts": "0",
        "num_readonly_unsigned_accounts": "1"
      },
      "static_account_keys": [
        "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
        "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD",
        "11111111111111111111111111111111"
      ],
      "address_table_lookups": [],
      "instructions": [
        {
          "program_id": "11111111111111111111111111111111",
          "accounts": [
            {
              "pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
              "is_signer": true,
              "is_writable": true
            },
            {
              "pubkey": "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD",
              "is_signer": false,
              "is_writable": true
            }
           ],
           "data": "AgAAAICWmAAAAAAA",
           "decoded_data": {
             "account_params": [
               {
                 "name": "from_account",
                 "localized_name": "From Account"
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
      ]
    }
  )");
  ASSERT_TRUE(expect_message_value);
  EXPECT_EQ(value, expect_message_value->GetDict());

  auto message_from_value = SolanaMessage::FromValue(value);
  EXPECT_EQ(message, message_from_value);

  std::vector<std::string> invalid_value_strings = {
      "{}",
      R"({"recent_blockhash": "recent blockhash", "fee_payer": "fee payer"})",
      R"({"recent_blockhash": "recent blockhash", "instructions": []})",
      R"({"fee_payer": "fee payer", "instructions": []})"};

  for (const auto& invalid_value_string : invalid_value_strings) {
    std::optional<base::Value> invalid_value =
        base::JSONReader::Read(invalid_value_string);
    ASSERT_TRUE(invalid_value) << ":" << invalid_value_string;
    EXPECT_FALSE(SolanaMessage::FromValue(invalid_value->GetDict()))
        << ":" << invalid_value_string;
  }

  // v0
  expect_message_value = base::JSONReader::Read(R"(
    {
      "version": 1,
      "recent_blockhash": "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6",
      "last_valid_block_height": "3090",
      "fee_payer": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
      "message_header": {
        "num_required_signatures": "1",
        "num_readonly_signed_accounts": "0",
        "num_readonly_unsigned_accounts": "1"
      },
      "static_account_keys": [
        "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
        "11111111111111111111111111111111"
      ],
      "address_table_lookups": [
        {
          "account_key": "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD",
          "base64_encoded_read_indexes": "AgQ=",
          "base64_encoded_write_indexes": "AwE="
        }
      ],
      "instructions": [
        {
          "program_id": "11111111111111111111111111111111",
          "accounts": [
            {
              "pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
              "is_signer": true,
              "is_writable": true
            },
            {
              "pubkey": "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD",
              "address_table_lookup_index": "1",
              "is_signer": false,
              "is_writable": true
            }
           ],
           "data": "AgAAAICWmAAAAAAA",
           "decoded_data": {
             "account_params": [
               {
                 "name": "from_account",
                 "localized_name": "From Account"
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
      ]
    }
  )");
  ASSERT_TRUE(expect_message_value);

  SolanaMessage message2 = GetTestV0Message();
  value = message2.ToValue();
  EXPECT_EQ(value, expect_message_value->GetDict());

  message_from_value = SolanaMessage::FromValue(value);
  EXPECT_EQ(message2, message_from_value);
}

TEST(SolanaMessageUnitTest, UsesDurableNonce) {
  // Mock AdvanceNonceAccount instruction.
  uint32_t instruction_type = static_cast<uint32_t>(
      mojom::SolanaSystemInstruction::kAdvanceNonceAccount);
  instruction_type = base::ByteSwapToLE32(instruction_type);

  std::vector<uint8_t> instruction_data(
      reinterpret_cast<uint8_t*>(&instruction_type),
      reinterpret_cast<uint8_t*>(&instruction_type) + sizeof(instruction_type));

  SolanaInstruction instruction = SolanaInstruction(
      mojom::kSolanaSystemProgramId,
      std::vector<SolanaAccountMeta>(
          {SolanaAccountMeta(kTestAccount, std::nullopt, false, true),
           SolanaAccountMeta(kFromAccount, std::nullopt, false, false),
           SolanaAccountMeta(kToAccount, std::nullopt, true, false)}),
      instruction_data);

  auto message1 = GetTestLegacyMessage();
  auto message2 = GetTestV0Message();
  for (auto* message : {&message1, &message2}) {
    EXPECT_FALSE(message->UsesDurableNonce());

    std::vector<SolanaInstruction> vec;
    vec.emplace_back(instruction);
    vec.emplace_back(message->instructions()[0]);
    message->SetInstructionsForTesting(vec);
    EXPECT_TRUE(message->UsesDurableNonce());
  }
}

}  // namespace brave_wallet
