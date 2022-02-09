/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_message.h"

#include <vector>

#include "base/test/gtest_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SolanaMessageUnitTest, Serialize) {
  // Test serializing a message for transfering SOL.
  std::string from_account = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
  std::string to_account = "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD";

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});

  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  SolanaMessage message(recent_blockhash, from_account, {instruction});
  std::vector<std::string> signers;
  auto message_bytes = message.Serialize(&signers);
  ASSERT_TRUE(message_bytes);
  EXPECT_EQ(signers, std::vector<std::string>({from_account}));

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
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";

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
  SolanaMessage message(recent_blockhash, account1,
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
  message = SolanaMessage(recent_blockhash, account1,
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
      SolanaMessage(recent_blockhash, account1, {instruction1, instruction2});
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

}  // namespace brave_wallet
