/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_transaction.h"

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

const char kMnemonic[] =
    "divide cruise upon flag harsh carbon filter merit once advice bright "
    "drive";

}  // namespace

class SolanaTransactionUnitTest : public testing::Test {
 public:
  SolanaTransactionUnitTest() {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    keyring_service_.reset(new KeyringService(&prefs_));
  }

  ~SolanaTransactionUnitTest() override = default;

  KeyringService* keyring_service() { return keyring_service_.get(); }

  static bool RestoreWallet(KeyringService* service,
                            const std::string& mnemonic,
                            const std::string& password,
                            bool is_legacy_brave_wallet) {
    bool success = false;
    base::RunLoop run_loop;
    service->RestoreWallet(mnemonic, password, is_legacy_brave_wallet,
                           base::BindLambdaForTesting([&](bool v) {
                             success = v;
                             run_loop.Quit();
                           }));
    run_loop.Run();
    return success;
  }

  static bool AddAccount(KeyringService* service,
                         const std::string& account_name,
                         mojom::CoinType coin) {
    bool success = false;
    base::RunLoop run_loop;
    service->AddAccount(account_name, coin,
                        base::BindLambdaForTesting([&](bool v) {
                          success = v;
                          run_loop.Quit();
                        }));
    run_loop.Run();
    return success;
  }

 private:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<KeyringService> keyring_service_;
};

TEST_F(SolanaTransactionUnitTest, GetSignedTransaction) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_wallet::features::kBraveWalletSolanaFeature);
  ASSERT_TRUE(RestoreWallet(keyring_service(), kMnemonic, "brave", false));

  ASSERT_TRUE(AddAccount(keyring_service(), "Account 1", mojom::CoinType::SOL));
  ASSERT_TRUE(AddAccount(keyring_service(), "Account 2", mojom::CoinType::SOL));

  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  SolanaTransaction transaction(recent_blockhash, from_account, {instruction});

  std::vector<uint8_t> expected_bytes = {
      // Signature compact array
      1,  // num of signatures
      // signature byte array
      75, 128, 163, 160, 211, 120, 107, 238, 150, 64, 174, 92, 74, 8, 2, 134,
      237, 91, 221, 42, 91, 63, 197, 4, 181, 31, 247, 134, 38, 14, 244, 79, 131,
      43, 237, 96, 10, 161, 181, 22, 196, 36, 116, 73, 185, 231, 151, 96, 119,
      245, 215, 94, 171, 25, 159, 127, 33, 119, 208, 242, 179, 63, 36, 15,
      // Message header
      1,  // num_required_signatures
      0,  // num_readonly_signed_accounts
      1,  // num_readonly_unsigned_accounts

      // Account addresses compact array
      3,  // account addresses array length
      // account_addresses[0]: base58-decoded from account
      161, 51, 89, 91, 115, 210, 217, 212, 76, 159, 171, 200, 40, 150, 157, 70,
      197, 71, 24, 44, 209, 108, 143, 4, 58, 251, 215, 62, 201, 172, 159, 197,
      // account_addresses[1]: base58-decoded to account
      255, 224, 228, 245, 94, 238, 23, 132, 206, 40, 82, 249, 219, 203, 103,
      158, 110, 219, 93, 249, 143, 134, 207, 172, 179, 76, 67, 6, 169, 164, 149,
      38,
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
  std::string expected_tx = base::Base64Encode(expected_bytes);
  EXPECT_EQ(transaction.GetSignedTransaction(keyring_service(), ""),
            expected_tx);

  // Test two signers.
  instruction = SolanaInstruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, true, true)},
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  transaction =
      SolanaTransaction(recent_blockhash, from_account, {instruction});

  expected_bytes = std::vector<uint8_t>({
      // Signature compact array
      2,  // num of signatures
      // first account's signature
      204, 127, 175, 133, 20, 97, 41, 39, 106, 79, 38, 41, 221, 89, 38, 223,
      218, 63, 117, 68, 237, 45, 169, 94, 53, 56, 233, 159, 107, 110, 171, 152,
      241, 104, 11, 121, 164, 73, 210, 252, 42, 235, 214, 82, 107, 225, 218, 70,
      128, 175, 10, 17, 45, 190, 13, 100, 169, 164, 104, 207, 112, 145, 133, 2,
      // second account's signature
      54, 115, 88, 109, 108, 123, 97, 39, 185, 100, 244, 248, 224, 182, 51, 40,
      54, 151, 223, 15, 86, 126, 161, 53, 72, 107, 159, 23, 72, 82, 18, 31, 99,
      52, 175, 135, 38, 202, 71, 215, 64, 171, 122, 99, 178, 217, 144, 109, 88,
      75, 198, 137, 92, 222, 109, 229, 52, 138, 101, 182, 42, 134, 216, 4,
      // Message header
      2,  // num_required_signatures
      0,  // num_readonly_signed_accounts
      1,  // num_readonly_unsigned_accounts

      // Account addresses compact array
      3,  // account addresses array length
      // account_addresses[0]: base58-decoded from account
      161, 51, 89, 91, 115, 210, 217, 212, 76, 159, 171, 200, 40, 150, 157, 70,
      197, 71, 24, 44, 209, 108, 143, 4, 58, 251, 215, 62, 201, 172, 159, 197,
      // account_addresses[1]: base58-decoded to account
      255, 224, 228, 245, 94, 238, 23, 132, 206, 40, 82, 249, 219, 203, 103,
      158, 110, 219, 93, 249, 143, 134, 207, 172, 179, 76, 67, 6, 169, 164, 149,
      38,
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
  });
  expected_tx = base::Base64Encode(expected_bytes);
  EXPECT_EQ(transaction.GetSignedTransaction(keyring_service(), ""),
            expected_tx);

  // Test key_service is nullptr.
  EXPECT_TRUE(transaction.GetSignedTransaction(nullptr, "").empty());

  std::vector<uint8_t> oversized_data(1232, 1);
  instruction = SolanaInstruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      oversized_data);
  transaction =
      SolanaTransaction(recent_blockhash, from_account, {instruction});
  EXPECT_TRUE(transaction.GetSignedTransaction(keyring_service(), "").empty());
}

TEST_F(SolanaTransactionUnitTest, FromToSolanaTxData) {
  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      data);
  SolanaTransaction transaction(recent_blockhash, from_account, {instruction});

  auto solana_tx_data = transaction.ToSolanaTxData();
  ASSERT_TRUE(solana_tx_data);
  EXPECT_EQ(solana_tx_data->recent_blockhash, recent_blockhash);
  EXPECT_EQ(solana_tx_data->fee_payer, from_account);

  ASSERT_EQ(solana_tx_data->instructions.size(), 1u);
  EXPECT_EQ(solana_tx_data->instructions[0]->program_id,
            kSolanaSystemProgramId);
  EXPECT_EQ(solana_tx_data->instructions[0]->data, data);

  ASSERT_EQ(solana_tx_data->instructions[0]->account_metas.size(), 2u);
  EXPECT_EQ(solana_tx_data->instructions[0]->account_metas[0]->pubkey,
            from_account);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[0]->is_signer);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[0]->is_writable);
  EXPECT_EQ(solana_tx_data->instructions[0]->account_metas[1]->pubkey,
            to_account);
  EXPECT_FALSE(solana_tx_data->instructions[0]->account_metas[1]->is_signer);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[1]->is_writable);

  auto transaction_from_solana_tx_data =
      SolanaTransaction::FromSolanaTxData(std::move(solana_tx_data));
  ASSERT_TRUE(transaction_from_solana_tx_data);
  EXPECT_EQ(*transaction_from_solana_tx_data, transaction);
}

TEST_F(SolanaTransactionUnitTest, FromToValue) {
  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      data);
  SolanaTransaction transaction(recent_blockhash, from_account, {instruction});

  base::Value value = transaction.ToValue();
  auto expect_tx_value = base::JSONReader::Read(R"(
      {
        "message": {
          "recent_blockhash": "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6",
          "fee_payer": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
          "instructions": [
            {
              "program_id": "11111111111111111111111111111111",
              "accounts": [
                {
                  "pubkey": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                  "is_signer": true,
                  "is_writable": true
                },
                {
                  "pubkey": "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV",
                  "is_signer": false,
                  "is_writable": true
                }
               ],
               "data": "AgAAAICWmAAAAAAA"
             }
          ]
        }
      }
  )");

  ASSERT_TRUE(expect_tx_value);
  EXPECT_EQ(value, *expect_tx_value);
  auto tx_from_value = SolanaTransaction::FromValue(value);
  EXPECT_EQ(tx_from_value, transaction);

  std::vector<std::string> invalid_value_strings = {"{}", "[]"};

  for (const auto& invalid_value_string : invalid_value_strings) {
    absl::optional<base::Value> invalid_value =
        base::JSONReader::Read(invalid_value_string);
    ASSERT_TRUE(invalid_value) << ":" << invalid_value_string;
    EXPECT_FALSE(SolanaMessage::FromValue(*invalid_value))
        << ":" << invalid_value_string;
  }
}

}  // namespace brave_wallet
