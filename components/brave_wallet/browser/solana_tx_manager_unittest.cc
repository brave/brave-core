/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_manager.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "base/base64.h"
#include "base/files/scoped_temp_dir.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/solana_block_tracker.h"
#include "brave/components/brave_wallet/browser/solana_instruction_data_decoder.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/solana_tx_meta.h"
#include "brave/components/brave_wallet/browser/solana_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

namespace brave_wallet {

class SolanaTxManagerUnitTest : public testing::Test {
 public:
  SolanaTxManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    tx_hash1_ =
        "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpR"
        "zrFmBV6UjKdiSZkQUW";
    tx_hash2_ =
        "5j7s6NiJS3JAkvgkoc18WVAsiSaci2pxB2A6ueCJP4tprA2TFg9wSyTLeYouxPBJEMzJin"
        "ENTkpA52YStRW5Dia7";
    latest_blockhash1_ = "EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N";
    latest_blockhash2_ = "FkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N";
    latest_blockhash3_ = "GkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N";
    last_valid_block_height1_ = 3090;
    last_valid_block_height2_ = 3290;
    last_valid_block_height3_ = 3490;

    SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_, "",
                   false, last_valid_block_height1_);
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    brave_wallet::RegisterProfilePrefsForMigration(prefs_.registry());
    json_rpc_service_ =
        std::make_unique<JsonRpcService>(shared_url_loader_factory_, &prefs_);
    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        &prefs_, &local_state_);
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    tx_service_ = std::make_unique<TxService>(
        json_rpc_service_.get(), nullptr, nullptr, keyring_service_.get(),
        &prefs_, temp_dir_.GetPath(),
        base::SequencedTaskRunner::GetCurrentDefault());
    WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());
    CreateWallet();

    sol_account_ = SolAccount(0);
  }

  const mojom::AccountIdPtr& sol_account() { return sol_account_; }

  void CreateWallet() {
    base::RunLoop run_loop;
    keyring_service_->CreateWallet(
        kMnemonicDivideCruise, "brave",
        base::BindLambdaForTesting([&run_loop](const std::string& mnemonic) {
          ASSERT_FALSE(mnemonic.empty());
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  mojom::AccountInfoPtr AddAccount() {
    return keyring_service_->AddAccountSync(
        mojom::CoinType::SOL, mojom::kSolanaKeyringId, "New Account");
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  mojom::AccountIdPtr SolAccount(size_t index) {
    return GetAccountUtils().EnsureSolAccount(index)->account_id->Clone();
  }

  void SetInterceptor(const std::string& latest_blockhash,
                      uint64_t last_valid_block_height,
                      const std::string& tx_hash,
                      const std::string& content = "",
                      bool get_signature_statuses = false,
                      uint64_t block_height = 0,
                      bool get_null_signature_statuses = false) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, latest_blockhash, tx_hash, content, get_signature_statuses,
         last_valid_block_height, block_height,
         get_null_signature_statuses](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          base::Value::Dict request_root =
              base::test::ParseJsonDict(request_string);

          std::string* method = request_root.FindString("method");
          ASSERT_TRUE(method);

          if (*method == "getLatestBlockhash") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "{\"context\":{\"slot\":1069},\"value\":{\"blockhash\":\"" +
                    latest_blockhash + "\", \"lastValidBlockHeight\":" +
                    std::to_string(last_valid_block_height) + "}}}");
          } else if (*method == "getBlockHeight") {
            url_loader_factory_.AddResponse(
                request.url.spec(), R"({"jsonrpc":"2.0", "id":1, "result":)" +
                                        std::to_string(block_height) + "}");
          } else if (*method == "sendTransaction") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" + tx_hash +
                    "\"}");
          } else if (*method == "getAccountInfo" ||
                     *method == "getFeeForMessage") {
            url_loader_factory_.AddResponse(request.url.spec(), content);
          } else if (*method == "getSignatureStatuses") {
            if (!get_signature_statuses) {
              url_loader_factory_.AddResponse(request.url.spec(), "",
                                              net::HTTP_REQUEST_TIMEOUT);
              return;
            }

            if (get_null_signature_statuses) {
              url_loader_factory_.AddResponse(request.url.spec(),
                                              R"({"jsonrpc":"2.0", "id":1,
                      "result": {
                         "context": {"slot": 82},
                         "value": [null, null]
                      }})");
              return;
            }

            const base::Value::List* params_list =
                request_root.FindList("params");
            ASSERT_TRUE(params_list && (*params_list)[0].is_list());
            const std::string* hash =
                (*params_list)[0].GetList()[0].GetIfString();
            ASSERT_TRUE(hash);
            std::string json;

            // The tx is stored as dict, so we can't gurantee the sequence of
            // hash in the request to be the same as submissions.
            if (*hash == tx_hash1_) {
              json = R"(
                {"jsonrpc":2.0, "id":1, "result":
                  {
                    "context": {"slot": 82},
                    "value": [
                      {
                        "slot": 100,
                        "confirmations": 10,
                        "err": null,
                        "confirmationStatus": "confirmed"
                      },
                      {
                        "slot": 72,
                        "confirmations": 0,
                        "err": null,
                        "confirmationStatus": "finalized"
                      }
                    ]
                  }
                }
              )";
            } else {
              json = R"(
                {"jsonrpc":2.0, "id":1, "result":
                  {
                    "context": {"slot": 82},
                    "value": [
                      {
                        "slot": 72,
                        "confirmations": 0,
                        "err": null,
                        "confirmationStatus": "finalized"
                      },
                      {
                        "slot": 100,
                        "confirmations": 10,
                        "err": null,
                        "confirmationStatus": "confirmed"
                      }
                    ]
                  }
                }
              )";
            }
            url_loader_factory_.AddResponse(request.url.spec(), json);
          }
        }));
  }

  SolanaTxManager* solana_tx_manager() {
    return static_cast<SolanaTxManager*>(
        tx_service_->GetTxManager(mojom::CoinType::SOL));
  }
  PrefService* prefs() { return &prefs_; }

  url::Origin GetOrigin() const {
    return url::Origin::Create(GURL("https://brave.com"));
  }

  void AddUnapprovedTransaction(const std::string& chain_id,
                                mojom::SolanaTxDataPtr solana_tx_data,
                                const mojom::AccountIdPtr& from,
                                std::string* meta_id) {
    AddUnapprovedTransaction(chain_id, std::move(solana_tx_data), from,
                             GetOrigin(), meta_id);
  }

  void AddUnapprovedTransaction(const std::string& chain_id,
                                mojom::SolanaTxDataPtr solana_tx_data,
                                const mojom::AccountIdPtr& from,
                                const std::optional<url::Origin>& origin,
                                std::string* meta_id) {
    auto tx_data_union =
        mojom::TxDataUnion::NewSolanaTxData(std::move(solana_tx_data));

    base::RunLoop run_loop;
    solana_tx_manager()->AddUnapprovedTransaction(
        chain_id, std::move(tx_data_union), from, origin,
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          ASSERT_TRUE(success);
          ASSERT_FALSE(id.empty());
          ASSERT_TRUE(err_message.empty());
          *meta_id = id;
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void ApproveTransaction(const std::string& chain_id,
                          const std::string& meta_id) {
    base::RunLoop run_loop;
    solana_tx_manager()->ApproveTransaction(
        chain_id, meta_id,
        base::BindLambdaForTesting([&](bool success,
                                       mojom::ProviderErrorUnionPtr error_union,
                                       const std::string& err_message) {
          ASSERT_TRUE(success);
          ASSERT_TRUE(error_union->is_solana_provider_error());
          ASSERT_EQ(error_union->get_solana_provider_error(),
                    mojom::SolanaProviderError::kSuccess);
          ASSERT_TRUE(err_message.empty());
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestMakeSystemProgramTransferTxData(
      const std::string& from,
      const std::string& to,
      uint64_t lamports,
      mojom::SolanaTxDataPtr expected_tx_data,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_err_message,
      mojom::SolanaTxDataPtr* tx_data_out = nullptr) {
    base::RunLoop run_loop;
    solana_tx_manager()->MakeSystemProgramTransferTxData(
        from, to, lamports,
        base::BindLambdaForTesting([&](mojom::SolanaTxDataPtr tx_data,
                                       mojom::SolanaProviderError error,
                                       const std::string& err_message) {
          if (tx_data_out) {
            *tx_data_out = std::move(tx_data);
          } else {
            EXPECT_EQ(expected_tx_data, tx_data);
          }
          EXPECT_EQ(expected_error, error);
          EXPECT_EQ(expected_err_message, err_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestMakeSystemProgramTransferTxData(
      const mojom::AccountIdPtr& from,
      const std::string& to,
      uint64_t lamports,
      mojom::SolanaTxDataPtr expected_tx_data,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_err_message,
      mojom::SolanaTxDataPtr* tx_data_out = nullptr) {
    TestMakeSystemProgramTransferTxData(
        from->address, to, lamports, std::move(expected_tx_data),
        std::move(expected_error), std::move(expected_err_message),
        tx_data_out);
  }

  void TestMakeTokenProgramTransferTxData(
      const std::string& chain_id,
      const std::string& spl_token_mint_address,
      const std::string& from_wallet_address,
      const std::string& to_wallet_address,
      uint64_t amount,
      mojom::SolanaTxDataPtr expected_tx_data,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_err_message) {
    base::RunLoop run_loop;
    solana_tx_manager()->MakeTokenProgramTransferTxData(
        chain_id, spl_token_mint_address, from_wallet_address,
        to_wallet_address, amount,
        base::BindLambdaForTesting([&](mojom::SolanaTxDataPtr tx_data,
                                       mojom::SolanaProviderError error,
                                       const std::string& err_message) {
          EXPECT_EQ(expected_tx_data, tx_data);
          EXPECT_EQ(expected_error, error);
          EXPECT_EQ(expected_err_message, err_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestMakeTxDataFromBase64EncodedTransaction(
      const std::string& encoded_transaction,
      const mojom::TransactionType tx_type,
      mojom::SolanaSendTransactionOptionsPtr send_options,
      mojom::SolanaTxDataPtr expected_tx_data,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_err_message) {
    base::RunLoop run_loop;
    solana_tx_manager()->MakeTxDataFromBase64EncodedTransaction(
        encoded_transaction, tx_type, std::move(send_options),
        base::BindLambdaForTesting([&](mojom::SolanaTxDataPtr tx_data,
                                       mojom::SolanaProviderError error,
                                       const std::string& err_message) {
          EXPECT_EQ(expected_tx_data, tx_data);
          EXPECT_EQ(expected_error, error);
          EXPECT_EQ(expected_err_message, err_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetEstimatedTxFee(const std::string& chain_id,
                             const std::string& tx_meta_id,
                             uint64_t expected_tx_fee,
                             mojom::SolanaProviderError expected_error,
                             const std::string& expected_err_message) {
    base::RunLoop run_loop;
    solana_tx_manager()->GetEstimatedTxFee(
        chain_id, tx_meta_id,
        base::BindLambdaForTesting([&](uint64_t tx_fee,
                                       mojom::SolanaProviderError error,
                                       const std::string& err_message) {
          EXPECT_EQ(expected_tx_fee, tx_fee);
          EXPECT_EQ(expected_error, error);
          EXPECT_EQ(expected_err_message, err_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetTransactionMessageToSign(
      const std::string& chain_id,
      const std::string& tx_meta_id,
      std::optional<std::vector<std::uint8_t>> expected_tx_message) {
    base::RunLoop run_loop;
    solana_tx_manager()->GetTransactionMessageToSign(
        chain_id, tx_meta_id,
        base::BindLambdaForTesting(
            [&](mojom::MessageToSignUnionPtr tx_message) {
              EXPECT_EQ(!!tx_message, expected_tx_message.has_value());
              if (expected_tx_message.has_value()) {
                ASSERT_TRUE(tx_message->is_message_bytes());
                std::optional<std::vector<std::uint8_t>> message_bytes =
                    tx_message->get_message_bytes();
                EXPECT_EQ(message_bytes, expected_tx_message);
              }
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestProcessSolanaHardwareSignature(
      const std::string& chain_id,
      const std::string& tx_meta_id,
      const std::vector<uint8_t>& signature,
      bool expected_result,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    solana_tx_manager()->ProcessSolanaHardwareSignature(
        chain_id, tx_meta_id, signature,
        base::BindLambdaForTesting([&](bool result,
                                       mojom::ProviderErrorUnionPtr error_union,
                                       const std::string& error_message) {
          EXPECT_EQ(expected_result, result);
          ASSERT_TRUE(error_union->is_solana_provider_error());
          EXPECT_EQ(error_union->get_solana_provider_error(), expected_error);
          EXPECT_EQ(expected_error_message, error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TxService> tx_service_;
  mojom::AccountIdPtr sol_account_;
  std::string tx_hash1_;
  std::string tx_hash2_;
  std::string latest_blockhash1_;
  std::string latest_blockhash2_;
  std::string latest_blockhash3_;
  uint64_t last_valid_block_height1_ = 0;
  uint64_t last_valid_block_height2_ = 0;
  uint64_t last_valid_block_height3_ = 0;
};

TEST_F(SolanaTxManagerUnitTest, AddAndApproveTransaction) {
  // Stop the block tracker explicitly to make sure it will be started when we
  // submit our pending transactions.
  solana_tx_manager()->GetSolanaBlockTracker()->Stop();

  const auto& from_account = sol_account();
  std::string from_account_address = from_account->address;

  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  std::vector<mojom::SolanaAccountMetaPtr> account_metas;
  auto account_meta1 =
      mojom::SolanaAccountMeta::New(from_account_address, nullptr, true, true);
  auto account_meta2 =
      mojom::SolanaAccountMeta::New(to_account, nullptr, false, true);
  account_metas.push_back(std::move(account_meta1));
  account_metas.push_back(std::move(account_meta2));

  auto instruction = mojom::SolanaInstruction::New(
      mojom::kSolanaSystemProgramId, std::move(account_metas), data, nullptr);
  std::vector<mojom::SolanaInstructionPtr> instructions;
  instructions.push_back(std::move(instruction));

  auto solana_tx_data = mojom::SolanaTxData::New(
      "", 0, from_account_address, to_account, "", 10000000, 0,
      mojom::TransactionType::SolanaSystemTransfer, std::move(instructions),
      mojom::SolanaMessageVersion::kLegacy,
      mojom::SolanaMessageHeader::New(1, 0, 1),
      std::vector<std::string>(
          {from_account_address, to_account, mojom::kSolanaSystemProgramId}),
      std::vector<mojom::SolanaMessageAddressTableLookupPtr>(), nullptr,
      nullptr);

  auto tx = SolanaTransaction::FromSolanaTxData(solana_tx_data.Clone());
  ASSERT_TRUE(tx);

  std::string meta_id1;
  AddUnapprovedTransaction(mojom::kSolanaMainnet, solana_tx_data.Clone(),
                           from_account, &meta_id1);

  auto tx_meta1 =
      solana_tx_manager()->GetTxForTesting(mojom::kSolanaMainnet, meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_EQ(tx_meta1->chain_id(), mojom::kSolanaMainnet);

  EXPECT_EQ(*tx_meta1->tx(), *tx);
  EXPECT_EQ(tx_meta1->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Unapproved);

  std::string meta_id2;
  AddUnapprovedTransaction(mojom::kSolanaMainnet, solana_tx_data.Clone(),
                           from_account, &meta_id2);
  auto tx_meta2 =
      solana_tx_manager()->GetTxForTesting(mojom::kSolanaMainnet, meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_EQ(tx_meta2->chain_id(), mojom::kSolanaMainnet);
  EXPECT_EQ(*tx_meta2->tx(), *tx);
  EXPECT_EQ(tx_meta2->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta2->from(), from_account);
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Unapproved);

  ApproveTransaction(mojom::kSolanaMainnet, meta_id1);
  // Wait for tx to be updated.
  base::RunLoop().RunUntilIdle();

  tx->message()->set_recent_blockhash(latest_blockhash1_);
  tx->message()->set_last_valid_block_height(last_valid_block_height1_);

  tx_meta1 =
      solana_tx_manager()->GetTxForTesting(mojom::kSolanaMainnet, meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_EQ(tx_meta1->chain_id(), mojom::kSolanaMainnet);
  EXPECT_EQ(*tx_meta1->tx(), *tx);
  EXPECT_EQ(tx_meta1->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx_meta1->tx_hash(), tx_hash1_);

  // Send another tx.
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash2_, "",
                 false, last_valid_block_height1_);
  ApproveTransaction(mojom::kSolanaMainnet, meta_id2);
  base::RunLoop().RunUntilIdle();

  tx_meta2 =
      solana_tx_manager()->GetTxForTesting(mojom::kSolanaMainnet, meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_EQ(tx_meta2->chain_id(), mojom::kSolanaMainnet);
  EXPECT_EQ(*tx_meta2->tx(), *tx);
  EXPECT_EQ(tx_meta2->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta2->from(), from_account);
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx_meta2->tx_hash(), tx_hash2_);

  // Fast forward to have block tracker run with current interceptor.
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));

  SetInterceptor(latest_blockhash2_, last_valid_block_height2_, tx_hash2_, "",
                 true, last_valid_block_height1_);

  // Fast forward again to have block tracker run with the new interceptor.
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));
  tx_meta1 =
      solana_tx_manager()->GetTxForTesting(mojom::kSolanaMainnet, meta_id1);
  EXPECT_EQ(mojom::TransactionStatus::Submitted, tx_meta1->status());
  EXPECT_EQ(tx_meta1->signature_status(),
            SolanaSignatureStatus(100u, 10u, "", "confirmed"));

  tx_meta2 =
      solana_tx_manager()->GetTxForTesting(mojom::kSolanaMainnet, meta_id2);
  EXPECT_EQ(mojom::TransactionStatus::Confirmed, tx_meta2->status());
  EXPECT_EQ(tx_meta2->signature_status(),
            SolanaSignatureStatus(72u, 0u, "", "finalized"));
}

TEST_F(SolanaTxManagerUnitTest, OfacSanctionedToAddress) {
  const auto& from = sol_account();
  const std::string ofac_sanctioned_to =
      "FepMPR8vahkJ98Fr22VKbfHU4f4PTAyi18PDZN2NooPb";
  auto* registry = BlockchainRegistry::GetInstance();
  registry->UpdateOfacAddressesList({base::ToLowerASCII(ofac_sanctioned_to)});
  TestMakeSystemProgramTransferTxData(
      from, ofac_sanctioned_to, 10000000, nullptr,
      mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_OFAC_RESTRICTION), nullptr);
}

TEST_F(SolanaTxManagerUnitTest, WalletOrigin) {
  const auto& from = sol_account();
  const std::string to = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  mojom::SolanaTxDataPtr system_transfer_data = nullptr;
  TestMakeSystemProgramTransferTxData(from, to, 10000000, nullptr,
                                      mojom::SolanaProviderError::kSuccess, "",
                                      &system_transfer_data);
  ASSERT_TRUE(system_transfer_data);

  std::string meta_id;
  AddUnapprovedTransaction(mojom::kSolanaMainnet,
                           std::move(system_transfer_data), from, std::nullopt,
                           &meta_id);

  auto tx_meta =
      solana_tx_manager()->GetTxForTesting(mojom::kSolanaMainnet, meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->chain_id(), mojom::kSolanaMainnet);
  EXPECT_EQ(tx_meta->origin(), url::Origin::Create(GURL("chrome://wallet")));
}

TEST_F(SolanaTxManagerUnitTest, SomeSiteOrigin) {
  const auto& from = sol_account();
  const std::string to = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  mojom::SolanaTxDataPtr system_transfer_data = nullptr;
  TestMakeSystemProgramTransferTxData(from, to, 10000000, nullptr,
                                      mojom::SolanaProviderError::kSuccess, "",
                                      &system_transfer_data);
  ASSERT_TRUE(system_transfer_data);

  std::string meta_id;
  AddUnapprovedTransaction(
      mojom::kSolanaMainnet, std::move(system_transfer_data), from,
      url::Origin::Create(GURL("https://some.site.com")), &meta_id);

  auto tx_meta =
      solana_tx_manager()->GetTxForTesting(mojom::kSolanaMainnet, meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->chain_id(), mojom::kSolanaMainnet);
  EXPECT_EQ(tx_meta->origin(),
            url::Origin::Create(GURL("https://some.site.com")));
}

TEST_F(SolanaTxManagerUnitTest, MakeSystemProgramTransferTxData) {
  const auto& from_account = sol_account();
  std::string from_account_address = from_account->address;
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  auto solana_account_meta1 =
      mojom::SolanaAccountMeta::New(from_account_address, nullptr, true, true);
  auto solana_account_meta2 =
      mojom::SolanaAccountMeta::New(to_account, nullptr, false, true);
  std::vector<mojom::SolanaAccountMetaPtr> account_metas;
  account_metas.push_back(std::move(solana_account_meta1));
  account_metas.push_back(std::move(solana_account_meta2));

  auto mojom_param = mojom::SolanaInstructionParam::New(
      "lamports", "Lamports", "10000000",
      mojom::SolanaInstructionParamType::kUint64);
  std::vector<mojom::SolanaInstructionParamPtr> mojom_params;
  mojom_params.emplace_back(std::move(mojom_param));
  auto mojom_decoded_data = mojom::DecodedSolanaInstructionData::New(
      static_cast<uint32_t>(mojom::SolanaSystemInstruction::kTransfer),
      solana_ins_data_decoder::GetMojomAccountParamsForTesting(
          mojom::SolanaSystemInstruction::kTransfer, std::nullopt),
      std::move(mojom_params));

  auto mojom_instruction = mojom::SolanaInstruction::New(
      mojom::kSolanaSystemProgramId, std::move(account_metas), data,
      std::move(mojom_decoded_data));

  std::vector<mojom::SolanaInstructionPtr> instructions;
  instructions.push_back(std::move(mojom_instruction));

  auto tx_data = mojom::SolanaTxData::New(
      "", 0, from_account_address, to_account, "", 10000000, 0,
      mojom::TransactionType::SolanaSystemTransfer, std::move(instructions),
      mojom::SolanaMessageVersion::kLegacy,
      mojom::SolanaMessageHeader::New(1, 0, 1),
      std::vector<std::string>(
          {from_account_address, to_account, mojom::kSolanaSystemProgramId}),
      std::vector<mojom::SolanaMessageAddressTableLookupPtr>(), nullptr,
      nullptr);

  TestMakeSystemProgramTransferTxData(from_account, to_account, 10000000,
                                      std::move(tx_data),
                                      mojom::SolanaProviderError::kSuccess, "");

  TestMakeSystemProgramTransferTxData(
      "", to_account, 10000000, nullptr,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  TestMakeSystemProgramTransferTxData(
      from_account, "", 10000000, nullptr,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(SolanaTxManagerUnitTest, MakeTokenProgramTransferTxData) {
  // Test receiving associated token account exists.
  std::string from_wallet_address =
      "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_wallet_address =
      "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string spl_token_mint_address =
      "AQoKYV7tYpTrFZN6P5oUufbQKAUr9mNYGe1TTJC9wajM";

  auto from_associated_token_account = SolanaKeyring::GetAssociatedTokenAccount(
      spl_token_mint_address, from_wallet_address);
  ASSERT_TRUE(from_associated_token_account);
  auto to_associated_token_account = SolanaKeyring::GetAssociatedTokenAccount(
      spl_token_mint_address, to_wallet_address);
  ASSERT_TRUE(to_associated_token_account);

  const std::vector<uint8_t> data = {3, 128, 150, 152, 0, 0, 0, 0, 0};

  auto solana_account_meta1 = mojom::SolanaAccountMeta::New(
      *from_associated_token_account, nullptr, false, true);
  auto solana_account_meta2 = mojom::SolanaAccountMeta::New(
      *to_associated_token_account, nullptr, false, true);
  auto solana_account_meta3 =
      mojom::SolanaAccountMeta::New(from_wallet_address, nullptr, true, false);
  std::vector<mojom::SolanaAccountMetaPtr> account_metas;
  account_metas.push_back(std::move(solana_account_meta1));
  account_metas.push_back(std::move(solana_account_meta2));
  account_metas.push_back(std::move(solana_account_meta3));

  auto mojom_param = mojom::SolanaInstructionParam::New(
      "amount", "Amount", "10000000",
      mojom::SolanaInstructionParamType::kUint64);
  std::vector<mojom::SolanaInstructionParamPtr> mojom_params;
  mojom_params.emplace_back(std::move(mojom_param));
  auto mojom_decoded_data = mojom::DecodedSolanaInstructionData::New(
      static_cast<uint32_t>(mojom::SolanaTokenInstruction::kTransfer),
      solana_ins_data_decoder::GetMojomAccountParamsForTesting(
          std::nullopt, mojom::SolanaTokenInstruction::kTransfer),
      std::move(mojom_params));

  auto mojom_transfer_instruction = mojom::SolanaInstruction::New(
      mojom::kSolanaTokenProgramId, std::move(account_metas), data,
      std::move(mojom_decoded_data));

  std::vector<mojom::SolanaInstructionPtr> instructions;
  instructions.push_back(mojom_transfer_instruction.Clone());
  auto tx_data = mojom::SolanaTxData::New(
      "", 0, from_wallet_address, to_wallet_address, spl_token_mint_address, 0,
      10000000, mojom::TransactionType::SolanaSPLTokenTransfer,
      std::move(instructions), mojom::SolanaMessageVersion::kLegacy,
      mojom::SolanaMessageHeader::New(1, 0, 1),
      std::vector<std::string>(
          {from_wallet_address, *from_associated_token_account,
           *to_associated_token_account, mojom::kSolanaTokenProgramId}),
      std::vector<mojom::SolanaMessageAddressTableLookupPtr>(), nullptr,
      nullptr);

  // Owner is the token program account.
  std::string json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value":{
          "data":["SEVMTE8gV09STEQ=","base64"],
          "executable":false,
          "lamports":88801034809120,
          "owner":"TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
          "rentEpoch":284
        }
      }
    }
  )";
  SetInterceptor("", 0, "", json);
  TestMakeTokenProgramTransferTxData(
      mojom::kSolanaMainnet, spl_token_mint_address, from_wallet_address,
      to_wallet_address, 10000000, std::move(tx_data),
      mojom::SolanaProviderError::kSuccess, "");

  account_metas.clear();
  auto solana_account_meta4 =
      mojom::SolanaAccountMeta::New(from_wallet_address, nullptr, true, true);
  auto solana_account_meta5 = mojom::SolanaAccountMeta::New(
      *to_associated_token_account, nullptr, false, true);
  auto solana_account_meta6 =
      mojom::SolanaAccountMeta::New(to_wallet_address, nullptr, false, false);
  auto solana_account_meta7 = mojom::SolanaAccountMeta::New(
      spl_token_mint_address, nullptr, false, false);
  auto solana_account_meta8 = mojom::SolanaAccountMeta::New(
      mojom::kSolanaSystemProgramId, nullptr, false, false);
  auto solana_account_meta9 = mojom::SolanaAccountMeta::New(
      mojom::kSolanaTokenProgramId, nullptr, false, false);
  account_metas.push_back(std::move(solana_account_meta4));
  account_metas.push_back(std::move(solana_account_meta5));
  account_metas.push_back(std::move(solana_account_meta6));
  account_metas.push_back(std::move(solana_account_meta7));
  account_metas.push_back(std::move(solana_account_meta8));
  account_metas.push_back(std::move(solana_account_meta9));

  instructions.clear();
  auto mojom_create_associated_account_instruction =
      mojom::SolanaInstruction::New(mojom::kSolanaAssociatedTokenProgramId,
                                    std::move(account_metas),
                                    std::vector<uint8_t>(), nullptr);
  instructions.push_back(
      std::move(mojom_create_associated_account_instruction));
  instructions.push_back(std::move(mojom_transfer_instruction));
  tx_data = mojom::SolanaTxData::New(
      "", 0, from_wallet_address, to_wallet_address, spl_token_mint_address, 0,
      10000000,
      mojom::TransactionType::
          SolanaSPLTokenTransferWithAssociatedTokenAccountCreation,
      std::move(instructions), mojom::SolanaMessageVersion::kLegacy,
      mojom::SolanaMessageHeader::New(1, 0, 5),
      std::vector<std::string>(
          {from_wallet_address, *to_associated_token_account,
           *from_associated_token_account,
           mojom::kSolanaAssociatedTokenProgramId, to_wallet_address,
           spl_token_mint_address, mojom::kSolanaSystemProgramId,
           mojom::kSolanaTokenProgramId}),
      std::vector<mojom::SolanaMessageAddressTableLookupPtr>(), nullptr,
      nullptr);

  // Test owner is not token program account.
  json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value":{
          "data":["SEVMTE8gV09STEQ=","base64"],
          "executable":false,
          "lamports":88801034809120,
          "owner":"11111111111111111111111111111111",
          "rentEpoch":284
        }
      }
    }
  )";
  SetInterceptor("", 0, "", json);
  TestMakeTokenProgramTransferTxData(
      mojom::kSolanaMainnet, spl_token_mint_address, from_wallet_address,
      to_wallet_address, 10000000, tx_data.Clone(),
      mojom::SolanaProviderError::kSuccess, "");

  // Test receiving associated token account does not exist.
  json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result":{
        "context":{"slot":123121238},
        "value":null
      }
    })";
  SetInterceptor("", 0, "", json);
  TestMakeTokenProgramTransferTxData(
      mojom::kSolanaMainnet, spl_token_mint_address, from_wallet_address,
      to_wallet_address, 10000000, std::move(tx_data),
      mojom::SolanaProviderError::kSuccess, "");

  // Empty addresses should be handled.
  TestMakeTokenProgramTransferTxData(
      mojom::kSolanaMainnet, "", to_wallet_address, spl_token_mint_address,
      10000000, nullptr, mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  TestMakeTokenProgramTransferTxData(
      mojom::kSolanaMainnet, from_wallet_address, "", spl_token_mint_address,
      10000000, nullptr, mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  TestMakeTokenProgramTransferTxData(
      mojom::kSolanaMainnet, from_wallet_address, to_wallet_address, "",
      10000000, nullptr, mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Test sending to OFAC Sanctioned address
  const std::string ofac_sanctioned_to =
      "FepMPR8vahkJ98Fr22VKbfHU4f4PTAyi18PDZN2NooPb";
  auto* registry = BlockchainRegistry::GetInstance();
  registry->UpdateOfacAddressesList({base::ToLowerASCII(ofac_sanctioned_to)});
  TestMakeTokenProgramTransferTxData(
      mojom::kSolanaMainnet, spl_token_mint_address, from_wallet_address,
      ofac_sanctioned_to, 10000000, std::move(tx_data),
      mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_OFAC_RESTRICTION));
}

TEST_F(SolanaTxManagerUnitTest, MakeTxDataFromBase64EncodedTransaction) {
  // OK: TX data from base64-encoded transaction.
  // Data from SolanaTransactionUnitTest.FromSignedTransactionBytes
  const std::string& encoded_transaction =
      "AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAAAAAAAAAAAAAABAAEDoTNZW3PS2dRMn6vIKJadRsVHGCzRbI8EOvvXPsmsn8X/"
      "4OT1Xu4XhM4oUvnby2eebttd+Y+"
      "Gz6yzTEMGqaSVJgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAg79TyWzB3v+"
      "wQ4jR2yoGqfCJjrmpBhFXewYqN6JAeFsBAgIAAQwCAAAAgJaYAAAAAAA=";
  const std::string& from_account =
      "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  const std::string& to_account =
      "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  const std::string& recent_blockhash =
      "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};
  auto solana_account_meta1 =
      mojom::SolanaAccountMeta::New(from_account, nullptr, true, true);
  auto solana_account_meta2 =
      mojom::SolanaAccountMeta::New(to_account, nullptr, false, true);
  std::vector<mojom::SolanaAccountMetaPtr> account_metas;
  account_metas.push_back(std::move(solana_account_meta1));
  account_metas.push_back(std::move(solana_account_meta2));

  auto mojom_param = mojom::SolanaInstructionParam::New(
      "lamports", "Lamports", "10000000",
      mojom::SolanaInstructionParamType::kUint64);
  std::vector<mojom::SolanaInstructionParamPtr> mojom_params;
  mojom_params.emplace_back(std::move(mojom_param));
  auto mojom_decoded_data = mojom::DecodedSolanaInstructionData::New(
      static_cast<uint32_t>(mojom::SolanaSystemInstruction::kTransfer),
      solana_ins_data_decoder::GetMojomAccountParamsForTesting(
          mojom::SolanaSystemInstruction::kTransfer, std::nullopt),
      std::move(mojom_params));

  auto mojom_instruction = mojom::SolanaInstruction::New(
      // Program ID
      mojom::kSolanaSystemProgramId,
      // Accounts
      std::move(account_metas),
      // Data
      data,
      // Decoded Data
      std::move(mojom_decoded_data));
  std::vector<mojom::SolanaInstructionPtr> instructions;
  instructions.push_back(std::move(mojom_instruction));
  auto send_options =
      SolanaTransaction::SendOptions(std::nullopt, std::nullopt, true);
  auto tx_data = mojom::SolanaTxData::New(
      recent_blockhash, 0, from_account, "", "", 0, 0,
      mojom::TransactionType::SolanaSwap, std::move(instructions),
      mojom::SolanaMessageVersion::kLegacy,
      mojom::SolanaMessageHeader::New(1, 0, 1),
      std::vector<std::string>(
          {from_account, to_account, mojom::kSolanaSystemProgramId}),
      std::vector<mojom::SolanaMessageAddressTableLookupPtr>(),
      send_options.ToMojomSendOptions(), nullptr);
  TestMakeTxDataFromBase64EncodedTransaction(
      encoded_transaction, mojom::TransactionType::SolanaSwap,
      send_options.ToMojomSendOptions(), std::move(tx_data),
      mojom::SolanaProviderError::kSuccess, "");

  // KO: empty message
  TestMakeTxDataFromBase64EncodedTransaction(
      "", mojom::TransactionType::SolanaSwap, nullptr, nullptr,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // KO: invalid base64 message
  TestMakeTxDataFromBase64EncodedTransaction(
      "not a base64 message", mojom::TransactionType::SolanaSwap, nullptr,
      nullptr, mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // KO: valid base64 message, but invalid transaction bytes
  TestMakeTxDataFromBase64EncodedTransaction(
      "YW5p", mojom::TransactionType::SolanaSwap, nullptr, nullptr,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(SolanaTxManagerUnitTest, GetEstimatedTxFee) {
  const auto& from = sol_account();
  const std::string to = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  mojom::SolanaTxDataPtr system_transfer_data = nullptr;
  TestMakeSystemProgramTransferTxData(from, to, 10000000, nullptr,
                                      mojom::SolanaProviderError::kSuccess, "",
                                      &system_transfer_data);
  ASSERT_TRUE(system_transfer_data);

  std::string system_transfer_meta_id;
  AddUnapprovedTransaction(mojom::kSolanaMainnet,
                           std::move(system_transfer_data), from,
                           &system_transfer_meta_id);
  ASSERT_FALSE(system_transfer_meta_id.empty());

  std::string json = R"({
      "jsonrpc": "2.0",
      "result": { "context": { "slot": 5068 }, "value": 18446744073709551615 },
      "id": 1
  })";

  // GetEstimatedTxFee without a valid latest blockhash being returned by
  // remote.
  SetInterceptor("", 0, "", json);
  TestGetEstimatedTxFee(mojom::kSolanaMainnet, system_transfer_meta_id, 0,
                        mojom::SolanaProviderError::kParsingError,
                        l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  std::string json2 = R"({
      "jsonrpc": "2.0",
      "result": { "context": { "slot": 5068 }, "value": null },
      "id": 1
  })";

  // GetEstimatedTxFee with latest blockhash and non-null tx fee from remote.
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, "", json);
  TestGetEstimatedTxFee(mojom::kSolanaMainnet, system_transfer_meta_id,
                        UINT64_MAX, mojom::SolanaProviderError::kSuccess, "");

  // GetEstimatedTxFee with cached blockhash and error at parsing tx fee.
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, "", "{}");
  TestGetEstimatedTxFee(mojom::kSolanaMainnet, system_transfer_meta_id, 0,
                        mojom::SolanaProviderError::kParsingError,
                        l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // GetEstimatedTxFee with cached blockhash and null tx fee from remote.
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, "", json2);
  TestGetEstimatedTxFee(mojom::kSolanaMainnet, system_transfer_meta_id, 0,
                        mojom::SolanaProviderError::kSuccess, "");

  // GetEstimatedTxFee with cached latest blockhash and non-null tx fee from
  // remote.
  SetInterceptor("", 0, "", json);
  TestGetEstimatedTxFee(mojom::kSolanaMainnet, system_transfer_meta_id,
                        UINT64_MAX, mojom::SolanaProviderError::kSuccess, "");
}

TEST_F(SolanaTxManagerUnitTest, DropTxWithInvalidBlockhash) {
  const auto& from = sol_account();
  const std::string to = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  mojom::SolanaTxDataPtr system_transfer_data = nullptr;
  TestMakeSystemProgramTransferTxData(from, to, 10000000, nullptr,
                                      mojom::SolanaProviderError::kSuccess, "",
                                      &system_transfer_data);
  ASSERT_TRUE(system_transfer_data);

  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_, "",
                 false, last_valid_block_height1_, false);
  std::string meta_id1;
  AddUnapprovedTransaction(mojom::kSolanaMainnet, system_transfer_data.Clone(),
                           from, &meta_id1);
  ASSERT_FALSE(meta_id1.empty());
  ApproveTransaction(mojom::kSolanaMainnet, meta_id1);

  SetInterceptor(latest_blockhash2_, last_valid_block_height2_, tx_hash2_, "",
                 false, last_valid_block_height1_, false);
  // Fast forward to have block tracker run with the new interceptor.
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));

  std::string meta_id2;
  AddUnapprovedTransaction(mojom::kSolanaMainnet, system_transfer_data.Clone(),
                           from, &meta_id2);
  ASSERT_FALSE(meta_id2.empty());
  ApproveTransaction(mojom::kSolanaMainnet, meta_id2);

  // Wait for tx to be updated.
  base::RunLoop().RunUntilIdle();

  // Check two submitted tx.
  auto pending_transactions =
      solana_tx_manager()->GetSolanaTxStateManager()->GetTransactionsByStatus(
          mojom::kSolanaMainnet, mojom::TransactionStatus::Submitted,
          std::nullopt);
  EXPECT_EQ(pending_transactions.size(), 2u);
  auto tx1 =
      solana_tx_manager()->GetTxForTesting(mojom::kSolanaMainnet, meta_id1);
  ASSERT_TRUE(tx1);
  EXPECT_EQ(tx1->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx1->tx()->message()->recent_blockhash(), latest_blockhash1_);
  EXPECT_EQ(tx1->tx()->message()->last_valid_block_height(),
            last_valid_block_height1_);

  auto tx2 =
      solana_tx_manager()->GetTxForTesting(mojom::kSolanaMainnet, meta_id2);
  ASSERT_TRUE(tx2);
  EXPECT_EQ(tx2->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx2->tx()->message()->recent_blockhash(), latest_blockhash2_);
  EXPECT_EQ(tx2->tx()->message()->last_valid_block_height(),
            last_valid_block_height2_);

  // Set Interceptor for return null signature statuses and block height only
  // valid for blockhash2.
  SetInterceptor(latest_blockhash3_, last_valid_block_height3_, "", "", true,
                 last_valid_block_height2_, true);
  // Fast forward to have block tracker run with the new interceptor.
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));
  // Wait for tx to be updated.
  base::RunLoop().RunUntilIdle();

  // Check blockhash1 should be dropped, blockhash2 stay as submitted.
  auto dropped_transactions =
      solana_tx_manager()->GetSolanaTxStateManager()->GetTransactionsByStatus(
          mojom::kSolanaMainnet, mojom::TransactionStatus::Dropped,
          std::nullopt);
  ASSERT_EQ(dropped_transactions.size(), 1u);
  EXPECT_EQ(dropped_transactions[0]->id(), meta_id1);

  pending_transactions =
      solana_tx_manager()->GetSolanaTxStateManager()->GetTransactionsByStatus(
          mojom::kSolanaMainnet, mojom::TransactionStatus::Submitted,
          std::nullopt);
  ASSERT_EQ(pending_transactions.size(), 1u);
  EXPECT_EQ(pending_transactions[0]->id(), meta_id2);
}

TEST_F(SolanaTxManagerUnitTest, GetTransactionMessageToSign) {
  // Unknown tx_meta_id yields null message
  TestGetTransactionMessageToSign(mojom::kSolanaMainnet, "Unknown",
                                  std::nullopt);
  std::vector<mojom::HardwareWalletAccountPtr> hw_infos;
  hw_infos.push_back(mojom::HardwareWalletAccount::New(
      "89DzXVKJ79xf9MkzTxatQESh5fcvsqBo9fCsbAXkCaZE", "path", "name 1",
      "Ledger", "device1", mojom::CoinType::SOL, mojom::KeyringId::kSolana));
  auto from = keyring_service_->AddHardwareAccountsSync(std::move(hw_infos))[0]
                  ->account_id->Clone();
  const std::string to = "148FvZU6e67eSB12wv7fXCH5FsTDW8tsxXo3nFuZhfCF";
  mojom::SolanaTxDataPtr system_transfer_data = nullptr;
  TestMakeSystemProgramTransferTxData(from, to, 1, nullptr,
                                      mojom::SolanaProviderError::kSuccess, "",
                                      &system_transfer_data);
  ASSERT_TRUE(system_transfer_data);
  std::string system_transfer_meta_id;
  AddUnapprovedTransaction(mojom::kSolanaMainnet,
                           std::move(system_transfer_data), from,
                           &system_transfer_meta_id);
  ASSERT_FALSE(system_transfer_meta_id.empty());

  // Invalid latest blockhash yields null message
  SetInterceptor("", 0, "");
  TestGetTransactionMessageToSign(mojom::kSolanaMainnet,
                                  system_transfer_meta_id, std::nullopt);

  // Valid latest blockhash yields valid transaction message to sign
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, "");
  std::optional<std::vector<std::uint8_t>> message = base::Base64Decode(
      "AQABA2odJRVUDnxVZv71pBNy0DZ/"
      "ui6dv1N37VgGEA+"
      "aezhZAMzywrLOSju1o9VJQ5KaB2lsblgqvdjtkDFlmZHz4KQAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAAAAAAAAAAAAAAAAAMxJDpKM0uOHO7ND/"
      "JXaMxecpg9Nv0bCw26RKZ1V1Oa5AQICAAEMAgAAAAEAAAAAAAAA");
  TestGetTransactionMessageToSign(mojom::kSolanaMainnet,
                                  system_transfer_meta_id, message);

  // Valid cached latest blockhash
  SetInterceptor("", 0, "", "");
  TestGetTransactionMessageToSign(mojom::kSolanaMainnet,
                                  system_transfer_meta_id, message);
}

TEST_F(SolanaTxManagerUnitTest, ProcessSolanaHardwareSignature) {
  // Unknown tx_meta_id is invalid
  TestProcessSolanaHardwareSignature(
      mojom::kSolanaMainnet, "Unknown", std::vector<uint8_t>(), false,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));

  std::vector<mojom::HardwareWalletAccountPtr> hw_infos;
  hw_infos.push_back(mojom::HardwareWalletAccount::New(
      "89DzXVKJ79xf9MkzTxatQESh5fcvsqBo9fCsbAXkCaZE", "path", "name 1",
      "Ledger", "device1", mojom::CoinType::SOL, mojom::KeyringId::kSolana));
  auto from = keyring_service_->AddHardwareAccountsSync(std::move(hw_infos))[0]
                  ->account_id.Clone();
  const std::string to = "148FvZU6e67eSB12wv7fXCH5FsTDW8tsxXo3nFuZhfCF";
  mojom::SolanaTxDataPtr system_transfer_data = nullptr;
  TestMakeSystemProgramTransferTxData(from, to, 1, nullptr,
                                      mojom::SolanaProviderError::kSuccess, "",
                                      &system_transfer_data);
  ASSERT_TRUE(system_transfer_data);
  std::string system_transfer_meta_id;
  AddUnapprovedTransaction(mojom::kSolanaMainnet,
                           std::move(system_transfer_data), from,
                           &system_transfer_meta_id);
  ASSERT_FALSE(system_transfer_meta_id.empty());

  std::string decoded_signature;
  std::string signature =
      "fJaHU9cDUoLsWLXJSPTgW3bAkhuZL319v2479igQtSp1ZyBjPi923jWkALg48uS75z5fp1JK"
      "1T4vdWi2D35fFEj";
  std::vector<uint8_t> signature_bytes;
  EXPECT_TRUE(Base58Decode(signature, &signature_bytes, kSolanaSignatureSize));

  // Blockhash not set is invalid
  TestProcessSolanaHardwareSignature(
      mojom::kSolanaMainnet, system_transfer_meta_id, signature_bytes, false,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  auto meta = solana_tx_manager()->GetTxForTesting(mojom::kSolanaMainnet,
                                                   system_transfer_meta_id);
  meta->tx()->message()->set_recent_blockhash(latest_blockhash1_);
  meta->tx()->message()->set_last_valid_block_height(last_valid_block_height1_);
  ASSERT_TRUE(
      solana_tx_manager()->GetSolanaTxStateManager()->AddOrUpdateTx(*meta));

  // Valid blockhash and valid number of signers is valid
  TestProcessSolanaHardwareSignature(
      mojom::kSolanaMainnet, system_transfer_meta_id, signature_bytes, true,
      mojom::SolanaProviderError::kSuccess, "");
}

}  // namespace brave_wallet
