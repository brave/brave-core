/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_message.h"

#include <vector>

#include "base/json/json_reader.h"
#include "base/test/gtest_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr char kFromAccount[] = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
constexpr char kToAccount[] = "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD";
constexpr char kRecentBlockhash[] =
    "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";

}  // namespace

TEST(SolanaMessageUnitTest, Serialize) {
  // Test serializing a message for transfering SOL.
  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(kFromAccount, true, true),
       SolanaAccountMeta(kToAccount, false, true)},
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});

  SolanaMessage message(kRecentBlockhash, kFromAccount, {instruction});
  std::vector<std::string> signers;
  auto message_bytes = message.Serialize(&signers);
  ASSERT_TRUE(message_bytes);
  EXPECT_EQ(signers, std::vector<std::string>({kFromAccount}));

  std::vector<uint8_t> expected_bytes = {
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
  EXPECT_EQ(message_bytes.value(), expected_bytes);
}

TEST(SolanaMessageUnitTest, GetUniqueAccountMetas) {
  std::vector<SolanaAccountMeta> unique_account_metas;

  std::string program1 = kSolanaSystemProgramId;
  std::string program2 = "Config1111111111111111111111111111111111111";
  std::string account1 = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
  std::string account2 = "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD";
  std::string account3 = "CiDwVBFgWV9E5MvXWoLgnEgn2hK7rJikbvfWavzAQz3";
  std::string account4 = "GcdayuLaLyrdmUu324nahyv33G5poQdLUEZ1nEytDeP";
  std::string account5 = "LX3EUdRUBUa3TbsYXLEUdj9J3prXkWXvLYSWyYyc2Jj";

  SolanaAccountMeta account1_fee_payer(account1, true, true);
  SolanaAccountMeta program1_non_signer_readonly(program1, false, false);

  // Test instructions with duplicate pubkeys.
  SolanaAccountMeta account2_non_signer_readonly(account2, false, false);
  SolanaInstruction instruction1(program1, {account2_non_signer_readonly}, {});
  SolanaMessage message(kRecentBlockhash, account1,
                        {instruction1, instruction1});
  message.GetUniqueAccountMetas(&unique_account_metas);
  // fee payer at first, program ID at last, and no duplicate account pubkeys.
  std::vector<SolanaAccountMeta> expected_account_metas = {
      account1_fee_payer, account2_non_signer_readonly,
      program1_non_signer_readonly};
  EXPECT_EQ(unique_account_metas, expected_account_metas);

  // Test account order.
  // signer-read-write -> signer-readonly -> non-signer-read-write ->
  // non-signer-readonly
  SolanaAccountMeta account3_non_signer_read_write(account3, false, true);
  SolanaAccountMeta account4_signer_readonly(account4, true, false);
  SolanaAccountMeta account5_signer_read_write(account5, true, true);
  SolanaAccountMeta program2_non_signer_readonly(program2, false, false);
  SolanaInstruction instruction2(
      program1, {account3_non_signer_read_write, account4_signer_readonly}, {});
  SolanaInstruction instruction3(program2, {account5_signer_read_write}, {});
  message = SolanaMessage(kRecentBlockhash, account1,
                          {instruction1, instruction2, instruction3});
  message.GetUniqueAccountMetas(&unique_account_metas);
  expected_account_metas = {
      account1_fee_payer,           account5_signer_read_write,
      account4_signer_readonly,     account3_non_signer_read_write,
      account2_non_signer_readonly, program1_non_signer_readonly,
      program2_non_signer_readonly};
  EXPECT_EQ(unique_account_metas, expected_account_metas);

  // Test writable being updated when readonly and read-write both present for
  // the same account pubkey.
  SolanaAccountMeta account2_signer_read_write(account2, true, true);
  SolanaAccountMeta account3_non_signer_readonly(account3, false, false);
  SolanaAccountMeta account4_non_signer_read_write(account4, false, true);
  SolanaAccountMeta account4_signer_read_write(account4, true, true);
  SolanaAccountMeta account5_signer_readonly(account5, true, false);

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
  message =
      SolanaMessage(kRecentBlockhash, account1, {instruction1, instruction2});
  message.GetUniqueAccountMetas(&unique_account_metas);
  expected_account_metas = {account1_fee_payer,
                            account5_signer_read_write,
                            account2_signer_read_write,
                            account4_signer_read_write,
                            account3_non_signer_read_write,
                            program1_non_signer_readonly};
  EXPECT_EQ(unique_account_metas, expected_account_metas);

  EXPECT_DCHECK_DEATH(message.GetUniqueAccountMetas(nullptr));
}

TEST(SolanaMessageUnitTest, ToSolanaTxData) {
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};
  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(kFromAccount, true, true),
       SolanaAccountMeta(kToAccount, false, true)},
      data);
  SolanaMessage message(kRecentBlockhash, kFromAccount, {instruction});

  auto solana_tx_data = message.ToSolanaTxData();
  ASSERT_TRUE(solana_tx_data);
  EXPECT_EQ(solana_tx_data->recent_blockhash, kRecentBlockhash);
  EXPECT_EQ(solana_tx_data->fee_payer, kFromAccount);

  ASSERT_EQ(solana_tx_data->instructions.size(), 1u);
  EXPECT_EQ(solana_tx_data->instructions[0]->program_id,
            kSolanaSystemProgramId);
  EXPECT_EQ(solana_tx_data->instructions[0]->data, data);

  ASSERT_EQ(solana_tx_data->instructions[0]->account_metas.size(), 2u);
  EXPECT_EQ(solana_tx_data->instructions[0]->account_metas[0]->pubkey,
            kFromAccount);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[0]->is_signer);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[0]->is_writable);
  EXPECT_EQ(solana_tx_data->instructions[0]->account_metas[1]->pubkey,
            kToAccount);
  EXPECT_FALSE(solana_tx_data->instructions[0]->account_metas[1]->is_signer);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[1]->is_writable);
}

TEST(SolanaMessageUnitTest, FromToValue) {
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};
  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(kFromAccount, true, true),
       SolanaAccountMeta(kToAccount, false, true)},
      data);
  SolanaMessage message(kRecentBlockhash, kFromAccount, {instruction});

  base::Value value = message.ToValue();
  auto expect_message_value = base::JSONReader::Read(R"(
    {
      "recent_blockhash": "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6",
      "fee_payer": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
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
           "data": "AgAAAICWmAAAAAAA"
          }
      ]
    }
  )");
  ASSERT_TRUE(expect_message_value);
  EXPECT_EQ(value, *expect_message_value);

  auto message_from_value = SolanaMessage::FromValue(value);
  EXPECT_EQ(message, message_from_value);

  std::vector<std::string> invalid_value_strings = {
      "{}", "[]",
      R"({"recent_blockhash": "recent blockhash", "fee_payer": "fee payer"})",
      R"({"recent_blockhash": "recent blockhash", "instructions": []})",
      R"({"fee_payer": "fee payer", "instructions": []})"};

  for (const auto& invalid_value_string : invalid_value_strings) {
    absl::optional<base::Value> invalid_value =
        base::JSONReader::Read(invalid_value_string);
    ASSERT_TRUE(invalid_value) << ":" << invalid_value_string;
    EXPECT_FALSE(SolanaMessage::FromValue(*invalid_value))
        << ":" << invalid_value_string;
  }
}

}  // namespace brave_wallet
