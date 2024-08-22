/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_transaction.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/test/bind.h"
#include "base/test/gtest_util.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr char kFromAccount[] = "3JjmwHtdYkPAqnvNY67aqumBCQUSzjjk3As4igo1oQ3X";
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
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        shared_url_loader_factory_, network_manager_.get(), &prefs_, nullptr);
    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        &prefs_, &local_state_);
  }

  ~SolanaTransactionUnitTest() override = default;

  KeyringService* keyring_service() { return keyring_service_.get(); }

  static bool RestoreWallet(KeyringService* service,
                            const std::string& mnemonic,
                            const std::string& password,
                            bool is_legacy_brave_wallet) {
    return service->RestoreWalletSync(mnemonic, password,
                                      is_legacy_brave_wallet);
  }

  static mojom::AccountInfoPtr AddAccount(KeyringService* service,
                                          const std::string& account_name) {
    return service->AddAccountSync(mojom::CoinType::SOL,
                                   mojom::kSolanaKeyringId, account_name);
  }

  void SetSelectedAccount(const mojom::AccountIdPtr& account_id) {
    bool success = false;
    base::RunLoop run_loop;
    keyring_service()->SetSelectedAccount(
        account_id.Clone(), base::BindLambdaForTesting([&](bool v) {
          success = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    ASSERT_TRUE(success);
    ASSERT_EQ(keyring_service()->GetSelectedSolanaDappAccount()->account_id,
              account_id);
  }

 private:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
};

TEST_F(SolanaTransactionUnitTest, GetSignedTransaction) {
  ASSERT_TRUE(
      RestoreWallet(keyring_service(), kMnemonicDivideCruise, "brave", false));

  auto selected_dapp_account = AddAccount(keyring_service(), "Account 1");
  ASSERT_TRUE(selected_dapp_account);
  auto from_account = AddAccount(keyring_service(), "Account 2");
  ASSERT_TRUE(from_account);
  ASSERT_EQ(from_account->address, kFromAccount);

  // Set selected account to be different from the one we expect to be used in
  // signing the transaction (from_account).
  SetSelectedAccount(selected_dapp_account->account_id);

  uint64_t last_valid_block_height = 3090;

  SolanaInstruction instruction(
      // Program ID
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, false, true)},
      // Data
      std::vector<uint8_t>({2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0}));
  auto msg = SolanaMessage::CreateLegacyMessage(
      kRecentBlockhash, last_valid_block_height, kFromAccount, {instruction});
  ASSERT_TRUE(msg);
  SolanaTransaction transaction(std::move(*msg));

  std::vector<uint8_t> expected_bytes = {
      // Signature compact array
      1,  // num of signatures
      // signature byte array
      238, 59, 100, 156, 219, 89, 5, 163, 197, 171, 27, 93, 195, 252, 221, 8,
      250, 21, 11, 112, 0, 143, 51, 69, 65, 30, 71, 140, 70, 36, 130, 68, 91,
      73, 53, 109, 43, 57, 87, 213, 42, 4, 1, 169, 62, 107, 250, 191, 110, 23,
      204, 119, 244, 122, 89, 15, 76, 9, 68, 85, 189, 116, 13, 15,
      // Message header
      1,  // num_required_signatures
      0,  // num_readonly_signed_accounts
      1,  // num_readonly_unsigned_accounts

      // Account addresses compact array
      3,  // account addresses array length
      // account_addresses[0]: base58-decoded from account
      34, 66, 156, 249, 123, 231, 124, 55, 10, 225, 0, 202, 134, 253, 103, 221,
      118, 243, 120, 79, 62, 189, 65, 164, 49, 98, 194, 182, 97, 111, 161, 254,
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
  EXPECT_EQ(transaction.GetSignedTransaction(keyring_service(),
                                             from_account->account_id),
            expected_tx);

  // Test three signers where one is fee payer and two signatures are from
  // sign_transaction_param. Create two transactions where signer accounts
  // order is different and use one as the encoded_serialized_message to check
  // if we sign the exact encoded_serialized_message and also respect their
  // signer/signature order in the passed in message.
  instruction = SolanaInstruction(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, true, true),
       SolanaAccountMeta(kTestAccount, std::nullopt, true, true)},
      {});
  SolanaInstruction instruction2(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kTestAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, true, true)},
      {});
  auto msg2 = SolanaMessage::CreateLegacyMessage(
      kRecentBlockhash, last_valid_block_height, kFromAccount, {instruction});
  ASSERT_TRUE(msg2);
  SolanaTransaction transaction2 = SolanaTransaction(std::move(*msg2));
  auto msg3 = SolanaMessage::CreateLegacyMessage(
      kRecentBlockhash, last_valid_block_height, kFromAccount, {instruction2});
  ASSERT_TRUE(msg3);
  SolanaTransaction transaction3 = SolanaTransaction(std::move(*msg3));

  auto sign_tx_param = mojom::SolanaSignTransactionParam::New();
  auto seriazlied_msg = transaction3.message()->Serialize(nullptr);
  ASSERT_TRUE(seriazlied_msg);
  sign_tx_param->encoded_serialized_msg = Base58Encode(*seriazlied_msg);

  std::vector<uint8_t> test_sig1(64, 1);
  std::vector<uint8_t> test_sig2(64, 2);
  sign_tx_param->signatures.push_back(
      mojom::SignaturePubkeyPair::New(std::nullopt, kFromAccount));
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
  std::vector<uint8_t> signature =
      keyring_service()->SignMessageBySolanaKeyring(from_account->account_id,
                                                    message_bytes);
  expected_bytes.insert(expected_bytes.end(), signature.begin(),
                        signature.end());
  expected_bytes.insert(expected_bytes.end(), test_sig2.begin(),
                        test_sig2.end());
  expected_bytes.insert(expected_bytes.end(), test_sig1.begin(),
                        test_sig1.end());
  expected_bytes.insert(expected_bytes.end(), message_bytes.begin(),
                        message_bytes.end());
  expected_tx = base::Base64Encode(expected_bytes);
  EXPECT_EQ(transaction2.GetSignedTransaction(keyring_service(),
                                              from_account->account_id),
            expected_tx);

  // Test when there are redundant signatures not in signers, we will only use
  // those in signers.
  sign_tx_param->signatures.push_back(mojom::SignaturePubkeyPair::New(
      std::vector<uint8_t>({64, 3}), kTestAccount2));
  transaction2.set_sign_tx_param(sign_tx_param.Clone());
  EXPECT_EQ(transaction2.GetSignedTransaction(keyring_service(),
                                              from_account->account_id),
            expected_tx);

  // Test when num of signatures available is less than signers.size in message,
  // the # of signature should still be the same to signers.size and unavaliable
  // signatures are filled with empty signatures.
  sign_tx_param->signatures.pop_back();
  sign_tx_param->signatures.pop_back();
  sign_tx_param->signatures.pop_back();
  transaction2.set_sign_tx_param(sign_tx_param.Clone());
  expected_bytes = std::vector<uint8_t>({3});  // # of signature
  expected_bytes.insert(expected_bytes.end(), signature.begin(),
                        signature.end());
  expected_bytes.insert(expected_bytes.end(), kSolanaSignatureSize * 2, 0);
  expected_bytes.insert(expected_bytes.end(), message_bytes.begin(),
                        message_bytes.end());
  EXPECT_EQ(transaction2.GetSignedTransaction(keyring_service(),
                                              from_account->account_id),
            base::Base64Encode(expected_bytes));

  // Test key_service is nullptr.
  EXPECT_TRUE(
      transaction2.GetSignedTransaction(nullptr, from_account->account_id)
          .empty());

  std::vector<uint8_t> oversized_data(1232, 1);
  instruction = SolanaInstruction(
      // Program ID
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, false, true)},
      oversized_data);
  auto msg4 = SolanaMessage::CreateLegacyMessage(
      kRecentBlockhash, last_valid_block_height, kFromAccount, {instruction});
  ASSERT_TRUE(msg4);
  SolanaTransaction transaction4 = SolanaTransaction(std::move(*msg4));
  EXPECT_TRUE(
      transaction4
          .GetSignedTransaction(keyring_service(), from_account->account_id)
          .empty());
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

  for (size_t i = 0; i < valid_signed_tx_with_two_signer.size(); ++i) {
    EXPECT_FALSE(SolanaTransaction::FromSignedTransactionBytes(
        std::vector<uint8_t>(valid_signed_tx_with_two_signer.begin() + i + 1,
                             valid_signed_tx_with_two_signer.end())));
    EXPECT_FALSE(SolanaTransaction::FromSignedTransactionBytes(
        std::vector<uint8_t>(valid_signed_tx_with_two_signer.begin(),
                             valid_signed_tx_with_two_signer.begin() + i)));
  }

  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  uint64_t last_valid_block_height = 0;
  auto instruction = SolanaInstruction(
      // Program ID
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, std::nullopt, true, true),
       SolanaAccountMeta(to_account, std::nullopt, true, true)},
      // Data
      std::vector<uint8_t>({2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0}));
  auto msg = SolanaMessage::CreateLegacyMessage(
      recent_blockhash, last_valid_block_height, from_account, {instruction});
  ASSERT_TRUE(msg);
  auto transaction = SolanaTransaction(std::move(*msg));
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
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, std::nullopt, true, true),
       SolanaAccountMeta(to_account, std::nullopt, false, true),
       SolanaAccountMeta(kTestAccount, 2, false, true),
       SolanaAccountMeta(kTestAccount2, 3, false, false)},
      data);
  std::vector<SolanaMessageAddressTableLookup> lookups;
  lookups.emplace_back(*SolanaAddress::FromBase58(kTestAccount),
                       std::vector<uint8_t>({0, 2}),
                       std::vector<uint8_t>({4, 6}));
  lookups.emplace_back(*SolanaAddress::FromBase58(kTestAccount2),
                       std::vector<uint8_t>({5, 7}),
                       std::vector<uint8_t>({1, 3}));
  std::vector<SolanaAddress> static_accounts = {
      *SolanaAddress::FromBase58(from_account),
      *SolanaAddress::FromBase58(to_account),
      *SolanaAddress::FromBase58(mojom::kSolanaSystemProgramId)};

  SolanaTransaction transaction(
      mojom::SolanaMessageVersion::kV0, recent_blockhash,
      last_valid_block_height, from_account, SolanaMessageHeader(1, 0, 1),
      std::move(static_accounts), {instruction}, std::move(lookups));
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
      mojom::SignaturePubkeyPair::New(std::nullopt, "public_key1"));
  sign_tx_param->signatures.push_back(mojom::SignaturePubkeyPair::New(
      std::vector<uint8_t>(kSolanaSignatureSize, 1), "public_key2"));
  transaction.set_sign_tx_param(sign_tx_param->Clone());

  auto solana_tx_data = transaction.ToSolanaTxData();
  ASSERT_TRUE(solana_tx_data);
  EXPECT_EQ(solana_tx_data->recent_blockhash, recent_blockhash);
  EXPECT_EQ(solana_tx_data->last_valid_block_height, last_valid_block_height);
  EXPECT_EQ(solana_tx_data->fee_payer, from_account);
  EXPECT_EQ(solana_tx_data->to_wallet_address, to_account);
  EXPECT_EQ(solana_tx_data->token_address, "");
  EXPECT_EQ(solana_tx_data->lamports, 10000000u);
  EXPECT_EQ(solana_tx_data->amount, 0u);
  EXPECT_EQ(solana_tx_data->tx_type,
            mojom::TransactionType::SolanaSystemTransfer);
  EXPECT_EQ(solana_tx_data->send_options, mojom_send_options);
  EXPECT_EQ(solana_tx_data->sign_transaction_param, sign_tx_param);

  ASSERT_EQ(solana_tx_data->instructions.size(), 1u);
  EXPECT_EQ(solana_tx_data->instructions[0]->program_id,
            mojom::kSolanaSystemProgramId);
  EXPECT_EQ(solana_tx_data->instructions[0]->data, data);

  ASSERT_EQ(solana_tx_data->instructions[0]->account_metas.size(), 4u);
  EXPECT_EQ(solana_tx_data->instructions[0]->account_metas[0]->pubkey,
            from_account);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[0]->is_signer);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[0]->is_writable);
  EXPECT_FALSE(solana_tx_data->instructions[0]
                   ->account_metas[0]
                   ->addr_table_lookup_index);

  EXPECT_EQ(solana_tx_data->instructions[0]->account_metas[1]->pubkey,
            to_account);
  EXPECT_FALSE(solana_tx_data->instructions[0]->account_metas[1]->is_signer);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[1]->is_writable);
  EXPECT_FALSE(solana_tx_data->instructions[0]
                   ->account_metas[1]
                   ->addr_table_lookup_index);

  EXPECT_EQ(solana_tx_data->instructions[0]->account_metas[2]->pubkey,
            kTestAccount);
  EXPECT_FALSE(solana_tx_data->instructions[0]->account_metas[2]->is_signer);
  EXPECT_TRUE(solana_tx_data->instructions[0]->account_metas[2]->is_writable);
  EXPECT_EQ(solana_tx_data->instructions[0]
                ->account_metas[2]
                ->addr_table_lookup_index->val,
            2u);

  EXPECT_EQ(solana_tx_data->instructions[0]->account_metas[3]->pubkey,
            kTestAccount2);
  EXPECT_FALSE(solana_tx_data->instructions[0]->account_metas[3]->is_signer);
  EXPECT_FALSE(solana_tx_data->instructions[0]->account_metas[3]->is_writable);
  EXPECT_EQ(solana_tx_data->instructions[0]
                ->account_metas[3]
                ->addr_table_lookup_index->val,
            3u);

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
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, std::nullopt, true, true),
       SolanaAccountMeta(to_account, std::nullopt, false, true),
       SolanaAccountMeta(kTestAccount, 2, false, true),
       SolanaAccountMeta(kTestAccount2, 3, false, false)},
      data);
  std::vector<SolanaMessageAddressTableLookup> lookups;
  lookups.emplace_back(*SolanaAddress::FromBase58(kTestAccount),
                       std::vector<uint8_t>({0, 2}),
                       std::vector<uint8_t>({4, 6}));
  lookups.emplace_back(*SolanaAddress::FromBase58(kTestAccount2),
                       std::vector<uint8_t>({5, 7}),
                       std::vector<uint8_t>({1, 3}));
  std::vector<SolanaAddress> static_accounts = {
      *SolanaAddress::FromBase58(from_account),
      *SolanaAddress::FromBase58(to_account),
      *SolanaAddress::FromBase58(mojom::kSolanaSystemProgramId)};

  SolanaTransaction transaction(
      mojom::SolanaMessageVersion::kV0, recent_blockhash,
      last_valid_block_height, from_account, SolanaMessageHeader(1, 0, 1),
      std::move(static_accounts), {instruction}, std::move(lookups));
  transaction.set_to_wallet_address(to_account);
  transaction.set_lamports(10000000u);
  transaction.set_tx_type(mojom::TransactionType::SolanaSystemTransfer);
  transaction.set_send_options(
      SolanaTransaction::SendOptions(1, "confirmed", true));

  auto sign_tx_param = mojom::SolanaSignTransactionParam::New();
  sign_tx_param->encoded_serialized_msg = "encoded_serialized_message";
  sign_tx_param->signatures.push_back(
      mojom::SignaturePubkeyPair::New(std::nullopt, "public_key1"));
  sign_tx_param->signatures.push_back(mojom::SignaturePubkeyPair::New(
      std::vector<uint8_t>(2, 1), "public_key2"));
  transaction.set_sign_tx_param(sign_tx_param->Clone());

  auto fee_estimation = mojom::SolanaFeeEstimation::New();
  fee_estimation->base_fee = 5000;
  fee_estimation->compute_units = 200;
  fee_estimation->fee_per_compute_unit = 25;
  transaction.set_fee_estimation(std::move(fee_estimation));

  base::Value::Dict value = transaction.ToValue();
  auto expect_tx_value = base::test::ParseJson(R"(
      {
        "message": {
          "version": 1,
          "recent_blockhash": "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6",
          "last_valid_block_height": "3090",
          "fee_payer": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
          "message_header": {
            "num_required_signatures": "1",
            "num_readonly_signed_accounts": "0",
            "num_readonly_unsigned_accounts": "1"
          },
          "static_account_keys": [
            "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
            "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV",
            "11111111111111111111111111111111"
          ],
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
                },
                {
                  "pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
                  "is_signer": false,
                  "is_writable": true,
                  "address_table_lookup_index": "2"
                },
                {
                  "pubkey": "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD",
                  "is_signer": false,
                  "is_writable": false,
                  "address_table_lookup_index": "3"
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
          ],
          "address_table_lookups": [
            {
              "account_key": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
              "base64_encoded_read_indexes": "BAY=",
              "base64_encoded_write_indexes": "AAI="
            },
            {
              "account_key": "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD",
              "base64_encoded_read_indexes": "AQM=",
              "base64_encoded_write_indexes": "BQc="
            }
          ]
        },
        "to_wallet_address": "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV",
        "spl_token_mint_address": "",
        "lamports": "10000000",
        "amount": "0",
        "tx_type": 6,
        "wired_tx": "",
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
        },
        "fee_estimation": {
          "base_fee": "5000",
          "compute_units": "200",
          "fee_per_compute_unit": "25"
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
  auto msg = SolanaMessage::CreateLegacyMessage(
      "", 0, "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8", {});
  ASSERT_TRUE(msg);
  auto tx = SolanaTransaction(std::move(*msg));
  int max = static_cast<int>(mojom::TransactionType::kMaxValue);
  const base::flat_set<mojom::TransactionType> valid_types = {
      mojom::TransactionType::Other,
      mojom::TransactionType::SolanaSystemTransfer,
      mojom::TransactionType::SolanaSPLTokenTransfer,
      mojom::TransactionType::
          SolanaSPLTokenTransferWithAssociatedTokenAccountCreation,
      mojom::TransactionType::SolanaDappSignAndSendTransaction,
      mojom::TransactionType::SolanaDappSignTransaction,
      mojom::TransactionType::SolanaSwap,
      mojom::TransactionType::SolanaCompressedNftTransfer};
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
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, std::nullopt, true, true),
       SolanaAccountMeta(to_account, std::nullopt, false, true)},
      // Data
      std::vector<uint8_t>({2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0}));
  auto msg =
      SolanaMessage::CreateLegacyMessage("", 0, from_account, {instruction});
  ASSERT_TRUE(msg);
  SolanaTransaction transaction(std::move(*msg));

  // Blockhash not available.
  EXPECT_TRUE(transaction.GetBase64EncodedMessage().empty());

  // Blockhash is set.
  transaction.message()->set_recent_blockhash(recent_blockhash);
  auto result = transaction.GetBase64EncodedMessage();
  auto serialized_msg = transaction.message_.Serialize(nullptr);
  ASSERT_TRUE(serialized_msg);
  EXPECT_EQ(result, base::Base64Encode(*serialized_msg));

  // Blockhash is stored in the message already.
  auto msg2 = SolanaMessage::CreateLegacyMessage(
      recent_blockhash, last_valid_block_height, from_account, {instruction});
  ASSERT_TRUE(msg2);
  SolanaTransaction transaction2(std::move(*msg2));
  result = transaction2.GetBase64EncodedMessage();
  serialized_msg = transaction2.message_.Serialize(nullptr);
  ASSERT_TRUE(serialized_msg);
  EXPECT_EQ(result, base::Base64Encode(*serialized_msg));
}

TEST_F(SolanaTransactionUnitTest, GetSerializedMessage) {
  SolanaInstruction ins1(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, true, true),
       SolanaAccountMeta(kTestAccount, std::nullopt, true, true)},
      {});
  SolanaInstruction ins2(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kTestAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, true, true)},
      {});

  auto msg1 = SolanaMessage::CreateLegacyMessage(kRecentBlockhash, 0,
                                                 kFromAccount, {ins1});
  ASSERT_TRUE(msg1);
  auto tx1 = SolanaTransaction(std::move(*msg1));
  auto msg2 = SolanaMessage::CreateLegacyMessage(kRecentBlockhash, 0,
                                                 kFromAccount, {ins2});
  ASSERT_TRUE(msg2);
  auto tx2 = SolanaTransaction(std::move(*msg2));

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
  ASSERT_TRUE(
      RestoreWallet(keyring_service(), kMnemonicDivideCruise, "brave", false));

  auto selected_dapp_account = AddAccount(keyring_service(), "Account 1");
  ASSERT_TRUE(selected_dapp_account);
  auto from_account = AddAccount(keyring_service(), "Account 2");
  ASSERT_TRUE(from_account);
  ASSERT_EQ(from_account->address, kFromAccount);

  // Set selected account to be different from the one we expect to be used in
  // signing the transaction (from_account).
  SetSelectedAccount(selected_dapp_account->account_id);

  // Empty message is invalid
  std::vector<uint8_t> signature_bytes;
  std::string signature =
      "fJaHU9cDUoLsWLXJSPTgW3bAkhuZL319v2479igQtSp1ZyBjPi923jWkALg48uS75z5fp1JK"
      "1T4vdWi2D35fFEj";
  EXPECT_TRUE(Base58Decode(signature, &signature_bytes, kSolanaSignatureSize));
  SolanaTransaction transaction(mojom::SolanaMessageVersion::kLegacy, "", 0, "",
                                SolanaMessageHeader(), {}, {}, {});
  EXPECT_EQ(transaction.GetSignedTransactionBytes(
                keyring_service(), from_account->account_id, &signature_bytes),
            std::nullopt);

  // Valid
  SolanaInstruction instruction_one_signer(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kTestAccount2, std::nullopt, false, true)},
      std::vector<uint8_t>({2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0}));
  auto message = SolanaMessage::CreateLegacyMessage(
      "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6", 0, kFromAccount,
      {instruction_one_signer});
  ASSERT_TRUE(message);
  SolanaTransaction transaction2(std::move(*message));
  EXPECT_NE(transaction2.GetSignedTransactionBytes(
                keyring_service(), from_account->account_id, &signature_bytes),
            std::nullopt);

  // Empty signature is invalid
  std::vector<uint8_t> empty_signature_bytes;
  EXPECT_EQ(
      transaction2.GetSignedTransactionBytes(
          keyring_service(), from_account->account_id, &empty_signature_bytes),
      std::nullopt);

  // Test empty signature will be appended for non-selected-account signers.
  SolanaInstruction instruction_three_signers(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kTestAccount2, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, true, true)},
      std::vector<uint8_t>({2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0}));
  message = SolanaMessage::CreateLegacyMessage(
      "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6", 0, kFromAccount,
      {instruction_three_signers});
  ASSERT_TRUE(message);
  SolanaTransaction transaction3(std::move(*message));
  std::vector<mojom::SignaturePubkeyPairPtr> sig_key_pairs;
  sig_key_pairs.emplace_back(
      mojom::SignaturePubkeyPair::New(std::nullopt, kFromAccount));
  sig_key_pairs.emplace_back(
      mojom::SignaturePubkeyPair::New(std::nullopt, kTestAccount2));
  sig_key_pairs.emplace_back(
      mojom::SignaturePubkeyPair::New(signature_bytes, kToAccount));
  auto seriazlied_msg = transaction3.message()->Serialize(nullptr);
  ASSERT_TRUE(seriazlied_msg);
  transaction3.set_sign_tx_param(mojom::SolanaSignTransactionParam::New(
      Base58Encode(*seriazlied_msg), std::move(sig_key_pairs)));
  auto signed_tx_bytes = transaction3.GetSignedTransactionBytes(
      keyring_service(), from_account->account_id, &signature_bytes);
  ASSERT_TRUE(signed_tx_bytes);
  std::vector<uint8_t> expect_signed_tx_bytes = {3};
  expect_signed_tx_bytes.insert(expect_signed_tx_bytes.end(),
                                signature_bytes.begin(), signature_bytes.end());
  expect_signed_tx_bytes.insert(expect_signed_tx_bytes.end(),
                                kSolanaSignatureSize, 0);
  expect_signed_tx_bytes.insert(expect_signed_tx_bytes.end(),
                                signature_bytes.begin(), signature_bytes.end());
  expect_signed_tx_bytes.insert(expect_signed_tx_bytes.end(),
                                seriazlied_msg->begin(), seriazlied_msg->end());
  EXPECT_EQ(*signed_tx_bytes, expect_signed_tx_bytes);

  transaction3.set_sign_tx_param(nullptr);
  std::vector<uint8_t> expect_signed_tx_bytes2 = {3};
  expect_signed_tx_bytes2.insert(expect_signed_tx_bytes2.end(),
                                 signature_bytes.begin(),
                                 signature_bytes.end());
  expect_signed_tx_bytes2.insert(expect_signed_tx_bytes2.end(),
                                 kSolanaSignatureSize * 2, 0);
  expect_signed_tx_bytes2.insert(expect_signed_tx_bytes2.end(),
                                 seriazlied_msg->begin(),
                                 seriazlied_msg->end());
  EXPECT_EQ(
      transaction3
          .GetSignedTransactionBytes(keyring_service(),
                                     from_account->account_id, &signature_bytes)
          .value(),
      expect_signed_tx_bytes2);

  // Test selected account is not the fee payer.
  SolanaInstruction ins_not_fee_payer(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kTestAccount, std::nullopt, true, true),
       SolanaAccountMeta(kFromAccount, std::nullopt, true, true)},
      std::vector<uint8_t>({2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0}));
  message = SolanaMessage::CreateLegacyMessage(
      kRecentBlockhash, 0, kTestAccount, {ins_not_fee_payer});
  ASSERT_TRUE(message);
  SolanaTransaction transaction4(std::move(*message));
  std::vector<uint8_t> passed_sig_bytes(kSolanaSignatureSize, 1);
  sig_key_pairs.clear();
  sig_key_pairs.emplace_back(
      mojom::SignaturePubkeyPair::New(passed_sig_bytes, kTestAccount));
  sig_key_pairs.emplace_back(
      mojom::SignaturePubkeyPair::New(std::nullopt, kFromAccount));
  seriazlied_msg = transaction4.message()->Serialize(nullptr);
  ASSERT_TRUE(seriazlied_msg);
  transaction4.set_sign_tx_param(mojom::SolanaSignTransactionParam::New(
      Base58Encode(*seriazlied_msg), std::move(sig_key_pairs)));

  expect_signed_tx_bytes = {2};  // 2 signatures
  std::vector<uint8_t> selected_account_sig =
      keyring_service()->SignMessageBySolanaKeyring(from_account->account_id,
                                                    *seriazlied_msg);
  expect_signed_tx_bytes.insert(expect_signed_tx_bytes.end(),
                                passed_sig_bytes.begin(),
                                passed_sig_bytes.end());
  expect_signed_tx_bytes.insert(expect_signed_tx_bytes.end(),
                                selected_account_sig.begin(),
                                selected_account_sig.end());
  expect_signed_tx_bytes.insert(expect_signed_tx_bytes.end(),
                                seriazlied_msg->begin(), seriazlied_msg->end());
  auto result = transaction4.GetSignedTransactionBytes(
      keyring_service(), from_account->account_id);
  ASSERT_TRUE(result);
  EXPECT_EQ(*result, expect_signed_tx_bytes);
}

TEST_F(SolanaTransactionUnitTest, IsPartialSigned) {
  auto msg = SolanaMessage::CreateLegacyMessage("", 0, kFromAccount, {});
  ASSERT_TRUE(msg);
  auto tx = SolanaTransaction(std::move(*msg));
  EXPECT_FALSE(tx.IsPartialSigned());

  auto param = mojom::SolanaSignTransactionParam::New(
      "encoded_serialized_message",
      std::vector<mojom::SignaturePubkeyPairPtr>());
  tx.set_sign_tx_param(param.Clone());
  EXPECT_FALSE(tx.IsPartialSigned());

  param->signatures.emplace_back(
      mojom::SignaturePubkeyPair::New(std::nullopt, kFromAccount));
  tx.set_sign_tx_param(param.Clone());
  EXPECT_FALSE(tx.IsPartialSigned());

  param->signatures.emplace_back(mojom::SignaturePubkeyPair::New(
      std::vector<uint8_t>(kSolanaSignatureSize, 0), kFromAccount));
  tx.set_sign_tx_param(param.Clone());
  EXPECT_FALSE(tx.IsPartialSigned());

  param->signatures.emplace_back(mojom::SignaturePubkeyPair::New(
      std::vector<uint8_t>(kSolanaSignatureSize, 1), kFromAccount));
  tx.set_sign_tx_param(param.Clone());
  EXPECT_TRUE(tx.IsPartialSigned());
}

TEST_F(SolanaTransactionUnitTest, GetUnsignedTransaction) {
  auto msg1 =
      SolanaMessage::CreateLegacyMessage(kRecentBlockhash, 0, kFromAccount, {});
  auto tx1 = SolanaTransaction(std::move(*msg1));
  EXPECT_EQ(tx1.GetUnsignedTransaction(), "");

  SolanaInstruction ins1(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, true, true),
       SolanaAccountMeta(kTestAccount, std::nullopt, true, true)},
      {});

  auto msg2 = SolanaMessage::CreateLegacyMessage(kRecentBlockhash, 0,
                                                 kFromAccount, {ins1});
  ASSERT_TRUE(msg2);
  auto tx2 = SolanaTransaction(std::move(*msg2));
  EXPECT_EQ(tx2.GetUnsignedTransaction(),
            "AwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMAAQQi"
            "Qpz5e+d8NwrhAMqG/WfddvN4Tz69QaQxYsK2YW+h/v/"
            "g5PVe7heEzihS+dvLZ55u2135j4bPrLNMQwappJUmItA1NksucDd7D+"
            "gJLbL8xD5AqdVCV8AQmGz+"
            "lLcnM8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIO/U8lswd7/"
            "sEOI0dsqBqnwiY65qQYRV3sGKjeiQHhbAQMDAAECAA==");

  // Test unsigned transaction over kSolanaMaxTransactionSize.
  std::vector<SolanaAccountMeta> large_accounts;
  for (size_t i = 0; i < 50; ++i) {
    large_accounts.emplace_back(kFromAccount, std::nullopt, true, true);
    large_accounts.emplace_back(kToAccount, std::nullopt, true, true);
  }

  std::vector<uint8_t> large_data(1000, 0xAA);

  SolanaInstruction large_instruction(mojom::kSolanaSystemProgramId,
                                      std::move(large_accounts), large_data,
                                      std::nullopt);

  auto large_message = SolanaMessage::CreateLegacyMessage(
      kRecentBlockhash, 0, kFromAccount, {large_instruction});
  ASSERT_TRUE(large_message);

  SolanaTransaction large_tx(std::move(*large_message));
  EXPECT_EQ(large_tx.GetUnsignedTransaction(), "");
}

}  // namespace brave_wallet
