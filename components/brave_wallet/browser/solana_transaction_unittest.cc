/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_transaction.h"

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/test/bind.h"
#include "base/test/gtest_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr char kMnemonic[] =
    "divide cruise upon flag harsh carbon filter merit once advice bright "
    "drive";
constexpr char kFromAccount[] = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
constexpr char kToAccount[] = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
constexpr char kTestAccount[] = "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
constexpr char kTestAccount2[] = "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD";
constexpr char kRecentBlockhash[] =
    "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";

}  // namespace

class SolanaTransactionUnitTest : public testing::Test {
 public:
  SolanaTransactionUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_.reset(
        new JsonRpcService(shared_url_loader_factory_, &prefs_));
    keyring_service_.reset(
        new KeyringService(json_rpc_service_.get(), &prefs_));
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
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
};

TEST_F(SolanaTransactionUnitTest, GetSignedTransaction) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_wallet::features::kBraveWalletSolanaFeature);
  ASSERT_TRUE(RestoreWallet(keyring_service(), kMnemonic, "brave", false));

  ASSERT_TRUE(AddAccount(keyring_service(), "Account 1", mojom::CoinType::SOL));
  ASSERT_TRUE(AddAccount(keyring_service(), "Account 2", mojom::CoinType::SOL));

  uint64_t last_valid_block_height = 3090;

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(kFromAccount, true, true),
       SolanaAccountMeta(kToAccount, false, true)},
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  SolanaTransaction transaction(kRecentBlockhash, last_valid_block_height,
                                kFromAccount, {instruction});

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
  EXPECT_EQ(transaction.GetSignedTransaction(keyring_service()), expected_tx);

  // Test three signers where one is fee payer and two signatures are from
  // sign_transaction_param. Create two transactions where signer accounts
  // order is different and use one as the encoded_serialized_message to check
  // if we sign the exact encoded_serialized_message and also respect their
  // signer/signature order in the passed in message.
  instruction = SolanaInstruction(kSolanaSystemProgramId,
                                  {SolanaAccountMeta(kFromAccount, true, true),
                                   SolanaAccountMeta(kToAccount, true, true),
                                   SolanaAccountMeta(kTestAccount, true, true)},
                                  {});
  SolanaInstruction instruction2(kSolanaSystemProgramId,
                                 {SolanaAccountMeta(kFromAccount, true, true),
                                  SolanaAccountMeta(kTestAccount, true, true),
                                  SolanaAccountMeta(kToAccount, true, true)},
                                 {});
  SolanaTransaction transaction2 = SolanaTransaction(
      kRecentBlockhash, last_valid_block_height, kFromAccount, {instruction});
  SolanaTransaction transaction3 = SolanaTransaction(
      kRecentBlockhash, last_valid_block_height, kFromAccount, {instruction2});

  auto sign_tx_param = mojom::SolanaSignTransactionParam::New();
  auto seriazlied_msg = transaction3.message()->Serialize(nullptr);
  ASSERT_TRUE(seriazlied_msg);
  sign_tx_param->encoded_serialized_msg = Base58Encode(*seriazlied_msg);

  std::vector<uint8_t> test_sig1(64, 1);
  std::vector<uint8_t> test_sig2(64, 2);
  sign_tx_param->signatures.push_back(
      mojom::SignaturePubkeyPair::New(absl::nullopt, kFromAccount));
  sign_tx_param->signatures.push_back(
      mojom::SignaturePubkeyPair::New(test_sig1, kToAccount));
  sign_tx_param->signatures.push_back(
      mojom::SignaturePubkeyPair::New(test_sig2, kTestAccount));
  transaction2.set_sign_tx_param(sign_tx_param.Clone());

  // Should have 3 signatures, 1 from signing the passed in serialized msg
  // using our keyring, and 2 from the signature passed in. Then the message
  // byte array from the passed in serialized msg.
  expected_bytes = std::vector<uint8_t>({3});  // # of signature
  std::vector<uint8_t> message_bytes;
  ASSERT_TRUE(Base58Decode(sign_tx_param->encoded_serialized_msg,
                           &message_bytes, kSolanaMaxTxSize, false));
  std::vector<uint8_t> signature = keyring_service()->SignMessage(
      mojom::kSolanaKeyringId, kFromAccount, message_bytes);
  expected_bytes.insert(expected_bytes.end(), signature.begin(),
                        signature.end());
  expected_bytes.insert(expected_bytes.end(), test_sig2.begin(),
                        test_sig2.end());
  expected_bytes.insert(expected_bytes.end(), test_sig1.begin(),
                        test_sig1.end());
  expected_bytes.insert(expected_bytes.end(), message_bytes.begin(),
                        message_bytes.end());
  expected_tx = base::Base64Encode(expected_bytes);
  EXPECT_EQ(transaction2.GetSignedTransaction(keyring_service()), expected_tx);

  // Test when there are redundant signatures not in signers, we will only use
  // those in signers.
  sign_tx_param->signatures.push_back(mojom::SignaturePubkeyPair::New(
      std::vector<uint8_t>({64, 3}), kTestAccount2));
  transaction2.set_sign_tx_param(sign_tx_param.Clone());
  EXPECT_EQ(transaction2.GetSignedTransaction(keyring_service()), expected_tx);

  // Test when num of signatures available is less than signers.size in message,
  // should use the actual number of signatures available.
  sign_tx_param->signatures.pop_back();
  sign_tx_param->signatures.pop_back();
  sign_tx_param->signatures.pop_back();
  transaction2.set_sign_tx_param(sign_tx_param.Clone());
  expected_bytes = std::vector<uint8_t>({1});  // # of signature
  expected_bytes.insert(expected_bytes.end(), signature.begin(),
                        signature.end());
  expected_bytes.insert(expected_bytes.end(), message_bytes.begin(),
                        message_bytes.end());
  EXPECT_EQ(transaction2.GetSignedTransaction(keyring_service()),
            base::Base64Encode(expected_bytes));

  // Test key_service is nullptr.
  EXPECT_TRUE(transaction2.GetSignedTransaction(nullptr).empty());

  std::vector<uint8_t> oversized_data(1232, 1);
  instruction = SolanaInstruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(kFromAccount, true, true),
       SolanaAccountMeta(kToAccount, false, true)},
      oversized_data);
  SolanaTransaction transaction4 = SolanaTransaction(
      kRecentBlockhash, last_valid_block_height, kFromAccount, {instruction});
  EXPECT_TRUE(transaction4.GetSignedTransaction(keyring_service()).empty());
}

TEST_F(SolanaTransactionUnitTest, FromSignedTransactionBytes) {
  EXPECT_FALSE(
      SolanaTransaction::FromSignedTransactionBytes(std::vector<uint8_t>()));
  // size exceeds kSolanaMaxTxSize
  EXPECT_FALSE(SolanaTransaction::FromSignedTransactionBytes(
      std::vector<uint8_t>(1234, 1)));

  // Data from SolanaTransactionUnitTest.GetSignedTransaction
  const std::vector<uint8_t> valid_signed_tx_with_two_signer(
      {2,   204, 127, 175, 133, 20,  97,  41,  39,  106, 79,  38,  41,  221,
       89,  38,  223, 218, 63,  117, 68,  237, 45,  169, 94,  53,  56,  233,
       159, 107, 110, 171, 152, 241, 104, 11,  121, 164, 73,  210, 252, 42,
       235, 214, 82,  107, 225, 218, 70,  128, 175, 10,  17,  45,  190, 13,
       100, 169, 164, 104, 207, 112, 145, 133, 2,   54,  115, 88,  109, 108,
       123, 97,  39,  185, 100, 244, 248, 224, 182, 51,  40,  54,  151, 223,
       15,  86,  126, 161, 53,  72,  107, 159, 23,  72,  82,  18,  31,  99,
       52,  175, 135, 38,  202, 71,  215, 64,  171, 122, 99,  178, 217, 144,
       109, 88,  75,  198, 137, 92,  222, 109, 229, 52,  138, 101, 182, 42,
       134, 216, 4,   2,   0,   1,   3,   161, 51,  89,  91,  115, 210, 217,
       212, 76,  159, 171, 200, 40,  150, 157, 70,  197, 71,  24,  44,  209,
       108, 143, 4,   58,  251, 215, 62,  201, 172, 159, 197, 255, 224, 228,
       245, 94,  238, 23,  132, 206, 40,  82,  249, 219, 203, 103, 158, 110,
       219, 93,  249, 143, 134, 207, 172, 179, 76,  67,  6,   169, 164, 149,
       38,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   131, 191, 83,  201, 108, 193, 222, 255, 176,
       67,  136, 209, 219, 42,  6,   169, 240, 137, 142, 185, 169, 6,   17,
       87,  123, 6,   42,  55,  162, 64,  120, 91,  1,   2,   2,   0,   1,
       12,  2,   0,   0,   0,   128, 150, 152, 0,   0,   0,   0,   0});
  const std::vector<uint8_t> signatures(
      {204, 127, 175, 133, 20,  97,  41,  39,  106, 79,  38,  41,  221,
       89,  38,  223, 218, 63,  117, 68,  237, 45,  169, 94,  53,  56,
       233, 159, 107, 110, 171, 152, 241, 104, 11,  121, 164, 73,  210,
       252, 42,  235, 214, 82,  107, 225, 218, 70,  128, 175, 10,  17,
       45,  190, 13,  100, 169, 164, 104, 207, 112, 145, 133, 2,   54,
       115, 88,  109, 108, 123, 97,  39,  185, 100, 244, 248, 224, 182,
       51,  40,  54,  151, 223, 15,  86,  126, 161, 53,  72,  107, 159,
       23,  72,  82,  18,  31,  99,  52,  175, 135, 38,  202, 71,  215,
       64,  171, 122, 99,  178, 217, 144, 109, 88,  75,  198, 137, 92,
       222, 109, 229, 52,  138, 101, 182, 42,  134, 216, 4});
  const std::vector<uint8_t> valid_signed_tx_with_one_empty_signature(
      {1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   1,   3,   161,
       51,  89,  91,  115, 210, 217, 212, 76,  159, 171, 200, 40,  150, 157,
       70,  197, 71,  24,  44,  209, 108, 143, 4,   58,  251, 215, 62,  201,
       172, 159, 197, 255, 224, 228, 245, 94,  238, 23,  132, 206, 40,  82,
       249, 219, 203, 103, 158, 110, 219, 93,  249, 143, 134, 207, 172, 179,
       76,  67,  6,   169, 164, 149, 38,  0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   131, 191, 83,
       201, 108, 193, 222, 255, 176, 67,  136, 209, 219, 42,  6,   169, 240,
       137, 142, 185, 169, 6,   17,  87,  123, 6,   42,  55,  162, 64,  120,
       91,  1,   2,   2,   0,   1,   12,  2,   0,   0,   0,   128, 150, 152,
       0,   0,   0,   0,   0});
  const std::vector<uint8_t> empty_signature(64, 0);

  for (size_t i = 1; i < valid_signed_tx_with_two_signer.size(); ++i) {
    EXPECT_FALSE(SolanaTransaction::FromSignedTransactionBytes(
        std::vector<uint8_t>(valid_signed_tx_with_two_signer.begin() + i,
                             valid_signed_tx_with_two_signer.end())));
  }

  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  uint64_t last_valid_block_height = 0;
  auto instruction = SolanaInstruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, true, true)},
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  auto transaction = SolanaTransaction(
      recent_blockhash, last_valid_block_height, from_account, {instruction});

  auto result = SolanaTransaction::FromSignedTransactionBytes(
      valid_signed_tx_with_two_signer);
  ASSERT_TRUE(result);
  // original transaction doesn't have signature
  EXPECT_NE(*result, transaction);
  EXPECT_EQ(*result->message(), *transaction.message());
  EXPECT_EQ(result->raw_signatures(), signatures);

  // Signed transaction bytes with empty signature
  result = SolanaTransaction::FromSignedTransactionBytes(
      valid_signed_tx_with_one_empty_signature);
  ASSERT_TRUE(result);
  EXPECT_EQ(*result->message(), *transaction.message());
  EXPECT_EQ(result->raw_signatures(), empty_signature);
}

TEST_F(SolanaTransactionUnitTest, FromToSolanaTxData) {
  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  uint64_t last_valid_block_height = 3090;
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      data);
  SolanaTransaction transaction(recent_blockhash, last_valid_block_height,
                                from_account, {instruction});
  transaction.set_to_wallet_address(to_account);
  transaction.set_lamports(10000000u);
  transaction.set_tx_type(mojom::TransactionType::SolanaSystemTransfer);
  transaction.set_send_options(
      SolanaTransaction::SendOptions(1, "confirmed", true));

  auto mojom_send_options = mojom::SolanaSendTransactionOptions::New(
      mojom::OptionalMaxRetries::New(1), "confirmed",
      mojom::OptionalSkipPreflight::New(true));

  auto sign_tx_param = mojom::SolanaSignTransactionParam::New();
  sign_tx_param->encoded_serialized_msg = "encoded_serialized_message";
  sign_tx_param->signatures.push_back(
      mojom::SignaturePubkeyPair::New(absl::nullopt, "public_key1"));
  sign_tx_param->signatures.push_back(mojom::SignaturePubkeyPair::New(
      std::vector<uint8_t>(kSolanaSignatureSize, 1), "public_key2"));
  transaction.set_sign_tx_param(sign_tx_param->Clone());

  auto solana_tx_data = transaction.ToSolanaTxData();
  ASSERT_TRUE(solana_tx_data);
  EXPECT_EQ(solana_tx_data->recent_blockhash, recent_blockhash);
  EXPECT_EQ(solana_tx_data->last_valid_block_height, last_valid_block_height);
  EXPECT_EQ(solana_tx_data->fee_payer, from_account);
  EXPECT_EQ(solana_tx_data->to_wallet_address, to_account);
  EXPECT_EQ(solana_tx_data->spl_token_mint_address, "");
  EXPECT_EQ(solana_tx_data->lamports, 10000000u);
  EXPECT_EQ(solana_tx_data->amount, 0u);
  EXPECT_EQ(solana_tx_data->tx_type,
            mojom::TransactionType::SolanaSystemTransfer);
  EXPECT_EQ(solana_tx_data->send_options, mojom_send_options);
  EXPECT_EQ(solana_tx_data->sign_transaction_param, sign_tx_param);

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
  uint64_t last_valid_block_height = 3090;
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      data);
  SolanaTransaction transaction(recent_blockhash, last_valid_block_height,
                                from_account, {instruction});
  transaction.set_to_wallet_address(to_account);
  transaction.set_lamports(10000000u);
  transaction.set_tx_type(mojom::TransactionType::SolanaSystemTransfer);
  transaction.set_send_options(
      SolanaTransaction::SendOptions(1, "confirmed", true));

  auto sign_tx_param = mojom::SolanaSignTransactionParam::New();
  sign_tx_param->encoded_serialized_msg = "encoded_serialized_message";
  sign_tx_param->signatures.push_back(
      mojom::SignaturePubkeyPair::New(absl::nullopt, "public_key1"));
  sign_tx_param->signatures.push_back(mojom::SignaturePubkeyPair::New(
      std::vector<uint8_t>(2, 1), "public_key2"));
  transaction.set_sign_tx_param(sign_tx_param->Clone());

  base::Value::Dict value = transaction.ToValue();
  auto expect_tx_value = base::test::ParseJson(R"(
      {
        "message": {
          "recent_blockhash": "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6",
          "last_valid_block_height": "3090",
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
        },
        "to_wallet_address": "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV",
        "spl_token_mint_address": "",
        "lamports": "10000000",
        "amount": "0",
        "tx_type": 6,
        "send_options": {
          "maxRetries": "1",
          "preflightCommitment": "confirmed",
          "skipPreflight": true
        },
        "sign_tx_param": {
          "encoded_serialized_msg": "encoded_serialized_message",
          "signatures": [
            {"public_key": "public_key1"},
            {"signature": "AQE=", "public_key": "public_key2"}
          ]
        }
      }
  )");

  EXPECT_EQ(value, expect_tx_value.GetDict());
  auto tx_from_value = SolanaTransaction::FromValue(value);
  EXPECT_EQ(*tx_from_value, transaction);

  std::vector<std::string> invalid_value_strings = {"{}"};

  for (const auto& invalid_value_string : invalid_value_strings) {
    auto invalid_value = base::test::ParseJson(invalid_value_string);
    EXPECT_FALSE(SolanaMessage::FromValue(invalid_value.GetDict()))
        << ":" << invalid_value_string;
  }
}

TEST_F(SolanaTransactionUnitTest, SendOptionsFromValueMaxRetries) {
  auto value =
      base::test::ParseJson(R"({"maxRetries": "18446744073709551615"})");
  auto options =
      SolanaTransaction::SendOptions::FromValue(std::move(value.GetDict()));
  EXPECT_EQ(options->max_retries, UINT64_MAX);
  value = base::test::ParseJson(R"({"maxRetries": 9007199254740991})");
  options =
      SolanaTransaction::SendOptions::FromValue(std::move(value.GetDict()));
  EXPECT_EQ(options->max_retries, kMaxSafeIntegerUint64);

  // Unexpected type or no maxRetries.
  value = base::test::ParseJson(R"({"maxRetries": {}})");
  options =
      SolanaTransaction::SendOptions::FromValue(std::move(value.GetDict()));
  EXPECT_FALSE(options->max_retries);
  value = base::test::ParseJson(R"({})");
  options =
      SolanaTransaction::SendOptions::FromValue(std::move(value.GetDict()));
  EXPECT_FALSE(options->max_retries);
}

TEST_F(SolanaTransactionUnitTest, SetTxType) {
  auto tx = SolanaTransaction(
      "", 0, "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8", {});
  int max = static_cast<int>(mojom::TransactionType::kMaxValue);
  const base::flat_set<mojom::TransactionType> valid_types = {
      mojom::TransactionType::Other,
      mojom::TransactionType::SolanaSystemTransfer,
      mojom::TransactionType::SolanaSPLTokenTransfer,
      mojom::TransactionType::
          SolanaSPLTokenTransferWithAssociatedTokenAccountCreation,
      mojom::TransactionType::SolanaDappSignAndSendTransaction,
      mojom::TransactionType::SolanaDappSignTransaction,
      mojom::TransactionType::SolanaSwap};
  for (int i = 0; i <= max; i++) {
    auto type = static_cast<mojom::TransactionType>(i);
    if (valid_types.contains(type)) {
      tx.set_tx_type(type);
      EXPECT_EQ(tx.tx_type(), type);
    } else {
      EXPECT_DCHECK_DEATH(tx.set_tx_type(type));
    }
  }
}

TEST_F(SolanaTransactionUnitTest, GetBase64EncodedMessage) {
  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  uint64_t last_valid_block_height = 3090;

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  SolanaTransaction transaction("", 0, from_account, {instruction});

  // Blockhash not available.
  EXPECT_TRUE(transaction.GetBase64EncodedMessage().empty());

  // Blockhash is set.
  transaction.message()->set_recent_blockhash(recent_blockhash);
  auto result = transaction.GetBase64EncodedMessage();
  auto serialized_msg = transaction.message_.Serialize(nullptr);
  ASSERT_TRUE(serialized_msg);
  EXPECT_EQ(result, base::Base64Encode(*serialized_msg));

  // Blockhash is stored in the message already.
  SolanaTransaction transaction2(recent_blockhash, last_valid_block_height,
                                 from_account, {instruction});
  result = transaction2.GetBase64EncodedMessage();
  serialized_msg = transaction2.message_.Serialize(nullptr);
  ASSERT_TRUE(serialized_msg);
  EXPECT_EQ(result, base::Base64Encode(*serialized_msg));
}

TEST_F(SolanaTransactionUnitTest, GetSerializedMessage) {
  SolanaInstruction ins1(kSolanaSystemProgramId,
                         {SolanaAccountMeta(kFromAccount, true, true),
                          SolanaAccountMeta(kToAccount, true, true),
                          SolanaAccountMeta(kTestAccount, true, true)},
                         {});
  SolanaInstruction ins2(kSolanaSystemProgramId,
                         {SolanaAccountMeta(kFromAccount, true, true),
                          SolanaAccountMeta(kTestAccount, true, true),
                          SolanaAccountMeta(kToAccount, true, true)},
                         {});

  auto tx1 = SolanaTransaction(kRecentBlockhash, 0, kFromAccount, {ins1});
  auto tx2 = SolanaTransaction(kRecentBlockhash, 0, kFromAccount, {ins2});

  // Should use message.Serialize result when sign_tx_param_ is null.
  auto expected_message_bytes = tx1.message()->Serialize(nullptr);
  ASSERT_TRUE(expected_message_bytes);
  EXPECT_EQ(tx1.GetSerializedMessage(),
            std::make_pair(*expected_message_bytes,
                           std::vector<std::string>(
                               {kFromAccount, kToAccount, kTestAccount})));

  // Should use sign_tx_param_.encoded_serialized_message and signers from it
  // if exists.
  auto expected_message_bytes2 = tx2.message()->Serialize(nullptr);
  ASSERT_TRUE(expected_message_bytes2);
  EXPECT_NE(expected_message_bytes, expected_message_bytes2);
  tx1.set_sign_tx_param(mojom::SolanaSignTransactionParam::New(
      Base58Encode(*expected_message_bytes2),
      std::vector<mojom::SignaturePubkeyPairPtr>()));
  EXPECT_EQ(tx1.GetSerializedMessage(),
            std::make_pair(*expected_message_bytes2,
                           std::vector<std::string>(
                               {kFromAccount, kTestAccount, kToAccount})));
}

TEST_F(SolanaTransactionUnitTest, GetSignedTransactionBytes) {
  // Empty message is invalid
  std::vector<uint8_t> signature_bytes;
  std::string signature =
      "fJaHU9cDUoLsWLXJSPTgW3bAkhuZL319v2479igQtSp1ZyBjPi923jWkALg48uS75z5fp1JK"
      "1T4vdWi2D35fFEj";
  EXPECT_TRUE(Base58Decode(signature, &signature_bytes, kSolanaSignatureSize));
  SolanaTransaction transaction("", 0, "", {});
  EXPECT_EQ(transaction.GetSignedTransactionBytes(keyring_service(),
                                                  &signature_bytes),
            absl::nullopt);

  // Valid
  SolanaInstruction instruction_one_signer(
      kSolanaSystemProgramId,
      {SolanaAccountMeta("3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw", true,
                         true),
       SolanaAccountMeta("3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD", false,
                         true)},
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  SolanaMessage message("9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6", 0,
                        "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
                        {instruction_one_signer});
  SolanaTransaction transaction2(std::move(message));
  EXPECT_NE(transaction2.GetSignedTransactionBytes(keyring_service(),
                                                   &signature_bytes),
            absl::nullopt);

  // Empty signature is invalid
  std::vector<uint8_t> empty_signature_bytes;
  EXPECT_EQ(transaction2.GetSignedTransactionBytes(keyring_service(),
                                                   &empty_signature_bytes),
            absl::nullopt);
}

}  // namespace brave_wallet
