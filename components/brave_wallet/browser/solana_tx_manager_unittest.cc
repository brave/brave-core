/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_manager.h"

#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "base/base64.h"
#include "base/files/scoped_temp_dir.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/solana_block_tracker.h"
#include "brave/components/brave_wallet/browser/solana_instruction_data_decoder.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/brave_wallet/browser/solana_test_utils.h"
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
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

namespace {

constexpr char kEncodedSerializedMessage[] =
    "FDmjJVJ5XUQPik2xqs7NqP7VdMkDXNWLimqTR8C2KstRHZAdRUoCMQr7LXUjQ6dSer9jfWWfbN"
    "XzMToAWzoQLWvgduNCLxSVWVuiVZzqGPwC8mWT4SAu5NDCC5VTWcSNWj4Q9HSvgQitodttQiQR"
    "3yQvRZJurNzub3SBK3umEqULkVJPYZJRCmPbXQm9ebPEXGYQRKrjiAt7";

constexpr char kMockGetFeeForMessageResponse[] = R"({
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value": 5000
      }
    })";

}  // namespace

namespace brave_wallet {

class MockTxStateManagerObserver : public TxStateManager::Observer {
 public:
  explicit MockTxStateManagerObserver(TxStateManager* tx_state_manager) {
    observation_.Observe(tx_state_manager);
  }

  MOCK_METHOD1(OnTransactionStatusChanged, void(mojom::TransactionInfoPtr));
  MOCK_METHOD1(OnNewUnapprovedTx, void(mojom::TransactionInfoPtr));

 private:
  base::ScopedObservation<TxStateManager, TxStateManager::Observer>
      observation_{this};
};

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
    fee_estimation1_ = mojom::SolanaFeeEstimation::New();
    fee_estimation1_->base_fee = 5000;
    fee_estimation1_->compute_units = 69017 + 300;
    fee_estimation1_->fee_per_compute_unit = 100;

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
        shared_url_loader_factory_, json_rpc_service_.get(), nullptr, nullptr,
        keyring_service_.get(), &prefs_, temp_dir_.GetPath(),
        base::SequencedTaskRunner::GetCurrentDefault());
    WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());
    CreateWallet();

    sol_account_ = SolAccount(0);
    to_account_ = SolAccount(1);
  }

  const mojom::AccountIdPtr& sol_account() { return sol_account_; }
  const mojom::AccountIdPtr& to_account() { return to_account_; }

  void CreateWallet() {
    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, "brave");
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

  void SetAccountInfoInterceptor(
      const std::map<std::string, std::string> account_owner_map) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, account_owner_map](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          base::Value::Dict request_root =
              base::test::ParseJsonDict(request_string);
          auto* params = request_root.FindList("params");
          ASSERT_TRUE(params && !params->empty() && (*params)[0].is_string());
          std::string account_address = (*params)[0].GetString();
          ASSERT_TRUE(account_owner_map.contains(account_address));

          if (account_owner_map.at(account_address).empty()) {
            url_loader_factory_.AddResponse(request.url.spec(), R"(
                {
                  "jsonrpc":"2.0","id":1,
                  "result":{
                    "context":{"slot":123121238},
                    "value":null
                  }
                })");
            return;
          }

          std::string json = R"(
              {
                "jsonrpc":"2.0","id":1,
                "result": {
                  "context":{"slot":123065869},
                    "value":{
                    "data":["SEVMTE8gV09STEQ=","base64"],
                    "executable":false,
                    "lamports":88801034809120,
                    "owner": "$1",
                    "rentEpoch":284
                  }
                }
              }
          )";
          url_loader_factory_.AddResponse(
              request.url.spec(),
              base::ReplaceStringPlaceholders(
                  json, {account_owner_map.at(account_address)}, nullptr));
        }));
  }

  void SetInterceptor(
      const std::string& latest_blockhash,
      uint64_t last_valid_block_height,
      const std::string& tx_hash,
      const std::string& content = "",
      bool get_signature_statuses = false,
      uint64_t block_height = 0,
      std::optional<std::string> mock_signature_value = std::nullopt,
      std::optional<base::flat_map<std::string, std::string>>
          rpc_method_responses = std::nullopt) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, latest_blockhash, tx_hash, content, get_signature_statuses,
         last_valid_block_height, block_height, mock_signature_value,
         rpc_method_responses](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          base::Value::Dict request_root =
              base::test::ParseJsonDict(request_string);

          std::string* method = request_root.FindString("method");
          ASSERT_TRUE(method);

          // Check if there's a custom response for the method
          if (rpc_method_responses && rpc_method_responses->contains(*method)) {
            url_loader_factory_.AddResponse(request.url.spec(),
                                            rpc_method_responses->at(*method));
            return;
          }

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
            send_transaction_calls_++;
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" + tx_hash +
                    "\"}");
          } else if (*method == "simulateTransaction") {
            url_loader_factory_.AddResponse(request.url.spec(), R"({
              "jsonrpc": "2.0",
              "result": {
                "context": {
                  "apiVersion": "1.17.25",
                  "slot": 259225005
                },
                "value": {
                  "accounts": null,
                  "err": null,
                  "logs": [
                    "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY invoke [1]",
                    "Program log: Instruction: Transfer",
                    "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY success"
                  ],
                  "returnData": null,
                  "unitsConsumed": 69017
                }
              },
              "id": 1
            })");
          } else if (*method == "getAccountInfo" ||
                     *method == "getFeeForMessage") {
            url_loader_factory_.AddResponse(request.url.spec(), content);
          } else if (*method == "getSignatureStatuses") {
            if (!get_signature_statuses) {
              url_loader_factory_.AddResponse(request.url.spec(), "",
                                              net::HTTP_REQUEST_TIMEOUT);
              return;
            }

            if (mock_signature_value) {
              url_loader_factory_.AddResponse(
                  request.url.spec(), base::ReplaceStringPlaceholders(
                                          R"({"jsonrpc": "2.0", "id":1,
                        "result": {"context": {"slot": 82}, "value": $1}})",
                                          {*mock_signature_value}, nullptr));
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
          } else if (*method == "getRecentPrioritizationFees") {
            url_loader_factory_.AddResponse(request.url.spec(), R"({
              "jsonrpc": "2.0",
              "result": [
                {
                  "prioritizationFee": 100,
                  "slot": 293251906
                },
                {
                  "prioritizationFee": 200,
                  "slot": 293251906
                },
                {
                  "prioritizationFee": 0,
                  "slot": 293251805
                }
              ],
              "id": 1
            })");
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

  void ApproveTransaction(const std::string& meta_id) {
    base::RunLoop run_loop;
    solana_tx_manager()->ApproveTransaction(
        meta_id, base::BindLambdaForTesting(
                     [&](bool success, mojom::ProviderErrorUnionPtr error_union,
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

  void WaitForUpdatePendingTransactions() { task_environment_.RunUntilIdle(); }

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

  mojom::SolanaTxDataPtr GetSystemProgramTransferTxData(
      const mojom::AccountIdPtr& from,
      const mojom::AccountIdPtr& to,
      uint64_t lamports = 10000000) {
    mojom::SolanaTxDataPtr tx_data;
    TestMakeSystemProgramTransferTxData(from, to->address, lamports, nullptr,
                                        mojom::SolanaProviderError::kSuccess,
                                        "", &tx_data);
    return tx_data;
  }

  void TestMakeTokenProgramTransferTxData(
      base::Location location,
      const std::string& chain_id,
      const std::string& spl_token_mint_address,
      const std::string& from_wallet_address,
      const std::string& to_wallet_address,
      uint64_t amount,
      mojom::SolanaTxDataPtr expected_tx_data,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_err_message) {
    base::RunLoop run_loop;
    SCOPED_TRACE(testing::Message() << location.ToString());
    solana_tx_manager()->MakeTokenProgramTransferTxData(
        chain_id, spl_token_mint_address, from_wallet_address,
        to_wallet_address, amount, 8 /* decimals */,
        base::BindLambdaForTesting([&](mojom::SolanaTxDataPtr tx_data,
                                       mojom::SolanaProviderError error,
                                       const std::string& err_message) {
          EXPECT_EQ(expected_tx_data, tx_data);
          if (expected_tx_data && tx_data) {
            EXPECT_EQ(expected_tx_data->tx_type, tx_data->tx_type);
            EXPECT_EQ(expected_tx_data->instructions, tx_data->instructions);
            EXPECT_EQ(expected_tx_data->message_header,
                      tx_data->message_header);
            EXPECT_EQ(expected_tx_data->static_account_keys,
                      tx_data->static_account_keys);
          }
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

  void TestGetTransactionMessageToSign(
      const std::string& tx_meta_id,
      std::optional<std::vector<std::uint8_t>> expected_tx_message) {
    base::RunLoop run_loop;
    solana_tx_manager()->GetTransactionMessageToSign(
        tx_meta_id,
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
      const std::string& tx_meta_id,
      const std::vector<uint8_t>& signature,
      bool expected_result,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    solana_tx_manager()->ProcessSolanaHardwareSignature(
        tx_meta_id, signature,
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

  void TestRetryTransaction(const base::Location& location,
                            const std::string& tx_meta_id,
                            bool expected_result,
                            const std::string& expected_err_message,
                            const std::string& expected_blockhash = "") {
    SCOPED_TRACE(testing::Message() << location.ToString());
    MockTxStateManagerObserver observer(
        solana_tx_manager()->GetSolanaTxStateManager());
    if (expected_result) {
      EXPECT_CALL(observer, OnNewUnapprovedTx(testing::_)).Times(1);
    }

    base::RunLoop run_loop;
    solana_tx_manager()->RetryTransaction(
        tx_meta_id,
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          EXPECT_EQ(expected_err_message, err_message);
          EXPECT_EQ(expected_result, success);
          if (expected_result) {
            EXPECT_NE(tx_meta_id, id);
            EXPECT_FALSE(id.empty());

            // Get the new tx and check some fields.
            auto tx_meta = solana_tx_manager()->GetTxForTesting(id);
            ASSERT_TRUE(tx_meta);
            EXPECT_EQ(tx_meta->status(), mojom::TransactionStatus::Unapproved);
            EXPECT_TRUE(tx_meta->tx()->raw_signatures().empty());
            EXPECT_EQ(tx_meta->tx()->message()->recent_blockhash(),
                      expected_blockhash);
            EXPECT_EQ(tx_meta->tx()->message()->last_valid_block_height(), 0u);
          } else {
            EXPECT_TRUE(id.empty());
          }
          run_loop.Quit();
        }));
    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(&observer);
  }

  void TestGetSolanaTxFeeEstimation(
      const std::string& chain_id,
      const std::string& tx_meta_id,
      mojom::SolanaFeeEstimationPtr expected_estimation,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    solana_tx_manager()->GetSolanaTxFeeEstimation(
        chain_id, tx_meta_id,
        base::BindLambdaForTesting([&](mojom::SolanaFeeEstimationPtr estimation,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(expected_error, expected_error);
          EXPECT_EQ(expected_error_message, error_message);
          EXPECT_EQ(expected_estimation, estimation);
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
  mojom::AccountIdPtr to_account_;
  std::string tx_hash1_;
  std::string tx_hash2_;
  std::string latest_blockhash1_;
  std::string latest_blockhash2_;
  std::string latest_blockhash3_;
  uint64_t last_valid_block_height1_ = 0;
  uint64_t last_valid_block_height2_ = 0;
  uint64_t last_valid_block_height3_ = 0;
  size_t send_transaction_calls_ = 0;
  mojom::SolanaFeeEstimationPtr fee_estimation1_;
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
      nullptr, nullptr);

  // First add a partially signed transaction - it should fetch a base fee
  // but not a priority fee (simulateTransaction + getRecentPrioritizationFees).
  auto param = mojom::SolanaSignTransactionParam::New(
      kEncodedSerializedMessage, std::vector<mojom::SignaturePubkeyPairPtr>());
  param->signatures.emplace_back(mojom::SignaturePubkeyPair::New(
      std::vector<uint8_t>(kSolanaSignatureSize, 1), sol_account()->address));
  solana_tx_data->sign_transaction_param = std::move(param);
  auto tx = SolanaTransaction::FromSolanaTxData(solana_tx_data.Clone());
  ASSERT_TRUE(tx);
  std::string meta_id1;
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_, std::nullopt);
  AddUnapprovedTransaction(mojom::kSolanaMainnet, solana_tx_data.Clone(),
                           from_account, &meta_id1);
  auto tx_meta1 = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  mojom::SolanaFeeEstimationPtr expected_estimate =
      mojom::SolanaFeeEstimation::New();
  expected_estimate->base_fee = 5000;
  expected_estimate->compute_units = 0;
  expected_estimate->fee_per_compute_unit = 0;
  tx->set_fee_estimation(expected_estimate.Clone());
  EXPECT_EQ(*tx_meta1->tx(), *tx);

  // Remove partial signature
  solana_tx_data->sign_transaction_param = nullptr;
  tx = SolanaTransaction::FromSolanaTxData(solana_tx_data.Clone());
  ASSERT_TRUE(tx);

  // When base fee fetching fails, the tx should not have a fee estimation.
  base::flat_map<std::string, std::string> responses;
  responses["getFeeForMessage"] = "invalid";
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_, "",
                 false, last_valid_block_height1_, std::nullopt, responses);
  AddUnapprovedTransaction(mojom::kSolanaMainnet, solana_tx_data.Clone(),
                           from_account, &meta_id1);
  tx_meta1 = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_FALSE(tx_meta1->tx()->fee_estimation());
  responses.clear();

  // When priority fee fetching fails (simulateTransaction), the tx should have
  // a fee estimation, but only base fee should be non zero.
  std::string meta_id2;
  responses["simulateTransaction"] = "invalid";
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_, std::nullopt, responses);
  AddUnapprovedTransaction(mojom::kSolanaMainnet, solana_tx_data.Clone(),
                           from_account, &meta_id2);
  auto tx_meta2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_TRUE(tx_meta2->tx()->fee_estimation());
  EXPECT_EQ(tx_meta2->tx()->fee_estimation()->base_fee, 5000U);
  EXPECT_EQ(tx_meta2->tx()->fee_estimation()->compute_units, 0U);
  EXPECT_EQ(tx_meta2->tx()->fee_estimation()->fee_per_compute_unit, 0U);
  responses.clear();

  // When priority fee fetching fails (getRecentPrioritizationFees),
  // the tx should have a fee estimation, but fee_per_compute_unit should
  // be the default.
  std::string meta_id3;
  responses["getRecentPrioritizationFees"] = "invalid";
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_, std::nullopt, responses);

  AddUnapprovedTransaction(mojom::kSolanaMainnet, solana_tx_data.Clone(),
                           from_account, &meta_id3);
  auto tx_meta3 = solana_tx_manager()->GetTxForTesting(meta_id3);
  ASSERT_TRUE(tx_meta3);
  EXPECT_TRUE(tx_meta3->tx()->fee_estimation());
  EXPECT_EQ(tx_meta3->tx()->fee_estimation()->base_fee, 5000U);
  EXPECT_EQ(tx_meta3->tx()->fee_estimation()->compute_units, 69317U);
  EXPECT_EQ(tx_meta3->tx()->fee_estimation()->fee_per_compute_unit, 1U);

  // When `everything is successful, the tx should have a fee estimation with
  // each of base_fee, compute_units and fee_per_compute_unit set from data in
  // RPC responses
  std::string meta_id4;
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_);
  AddUnapprovedTransaction(mojom::kSolanaMainnet, solana_tx_data.Clone(),
                           from_account, &meta_id4);

  auto tx_meta4 = solana_tx_manager()->GetTxForTesting(meta_id4);
  ASSERT_TRUE(tx_meta4);
  EXPECT_EQ(tx_meta4->chain_id(), mojom::kSolanaMainnet);
  tx->message()->AddPriorityFee(
      69017 + 300,
      100);  // Added priority automatically in AddUnapprovedTransaction
  tx->set_fee_estimation(fee_estimation1_.Clone());
  EXPECT_EQ(*tx_meta4->tx(), *tx);
  EXPECT_EQ(tx_meta4->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta4->from(), from_account);
  EXPECT_EQ(tx_meta4->status(), mojom::TransactionStatus::Unapproved);

  std::string meta_id5;
  AddUnapprovedTransaction(mojom::kSolanaMainnet, solana_tx_data.Clone(),
                           from_account, &meta_id5);
  auto tx_meta5 = solana_tx_manager()->GetTxForTesting(meta_id5);
  ASSERT_TRUE(tx_meta5);
  EXPECT_EQ(tx_meta5->chain_id(), mojom::kSolanaMainnet);
  EXPECT_EQ(*tx_meta5->tx(), *tx);
  EXPECT_EQ(tx_meta5->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta5->from(), from_account);
  EXPECT_EQ(tx_meta5->status(), mojom::TransactionStatus::Unapproved);

  ApproveTransaction(meta_id4);
  WaitForUpdatePendingTransactions();

  tx->message()->set_recent_blockhash(latest_blockhash1_);
  tx->message()->set_last_valid_block_height(last_valid_block_height1_);
  tx->set_wired_tx(
      tx->GetSignedTransaction(keyring_service_.get(), from_account));

  tx_meta4 = solana_tx_manager()->GetTxForTesting(meta_id4);
  ASSERT_TRUE(tx_meta4);
  EXPECT_EQ(tx_meta4->chain_id(), mojom::kSolanaMainnet);
  EXPECT_EQ(*tx_meta4->tx(), *tx);
  EXPECT_EQ(tx_meta4->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta4->from(), from_account);
  EXPECT_EQ(tx_meta4->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx_meta4->tx_hash(), tx_hash1_);

  // Send another tx.
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash2_, "",
                 false, last_valid_block_height1_);
  ApproveTransaction(meta_id5);
  WaitForUpdatePendingTransactions();

  tx_meta5 = solana_tx_manager()->GetTxForTesting(meta_id5);
  ASSERT_TRUE(tx_meta5);
  EXPECT_EQ(tx_meta5->chain_id(), mojom::kSolanaMainnet);
  EXPECT_EQ(*tx_meta5->tx(), *tx);
  EXPECT_EQ(tx_meta5->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta5->from(), from_account);
  EXPECT_EQ(tx_meta5->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx_meta5->tx_hash(), tx_hash2_);

  // Fast forward to have block tracker run with current interceptor.
  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));

  SetInterceptor(latest_blockhash2_, last_valid_block_height2_, tx_hash2_, "",
                 true, last_valid_block_height1_);

  // Fast forward again to have block tracker run with the new interceptor.
  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));
  tx_meta4 = solana_tx_manager()->GetTxForTesting(meta_id4);
  EXPECT_EQ(mojom::TransactionStatus::Submitted, tx_meta4->status());
  EXPECT_EQ(tx_meta4->signature_status(),
            SolanaSignatureStatus(100u, 10u, "", "confirmed"));

  tx_meta5 = solana_tx_manager()->GetTxForTesting(meta_id5);
  EXPECT_EQ(mojom::TransactionStatus::Confirmed, tx_meta5->status());
  EXPECT_EQ(tx_meta5->signature_status(),
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

  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_);

  std::string meta_id;
  AddUnapprovedTransaction(mojom::kSolanaMainnet,
                           std::move(system_transfer_data), from, std::nullopt,
                           &meta_id);

  auto tx_meta = solana_tx_manager()->GetTxForTesting(meta_id);
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

  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_);
  std::string meta_id;
  AddUnapprovedTransaction(
      mojom::kSolanaMainnet, std::move(system_transfer_data), from,
      url::Origin::Create(GURL("https://some.site.com")), &meta_id);

  auto tx_meta = solana_tx_manager()->GetTxForTesting(meta_id);
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
      nullptr, nullptr);

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

class TokenProgramTest
    : public SolanaTxManagerUnitTest,
      public testing::WithParamInterface<mojom::SPLTokenProgram> {};

TEST_P(TokenProgramTest, MakeTokenProgramTransferTxData) {
  mojom::SPLTokenProgram token_program = GetParam();
  const std::string token_program_id =
      SPLTokenProgramToProgramID(token_program);

  // Test receiving associated token account exists.
  std::string from_wallet_address =
      "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_wallet_address =
      "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string spl_token_mint_address =
      "AQoKYV7tYpTrFZN6P5oUufbQKAUr9mNYGe1TTJC9wajM";

  auto from_associated_token_account = SolanaKeyring::GetAssociatedTokenAccount(
      spl_token_mint_address, from_wallet_address, token_program);
  ASSERT_TRUE(from_associated_token_account);
  auto to_associated_token_account = SolanaKeyring::GetAssociatedTokenAccount(
      spl_token_mint_address, to_wallet_address, token_program);
  ASSERT_TRUE(to_associated_token_account);

  const std::vector<uint8_t> data = {12, 128, 150, 152, 0, 0, 0, 0, 0, 8};

  auto solana_account_meta1 = mojom::SolanaAccountMeta::New(
      *from_associated_token_account, nullptr, false, true);
  auto mint_account_meta = mojom::SolanaAccountMeta::New(spl_token_mint_address,
                                                         nullptr, false, false);
  auto solana_account_meta2 = mojom::SolanaAccountMeta::New(
      *to_associated_token_account, nullptr, false, true);
  auto solana_account_meta3 =
      mojom::SolanaAccountMeta::New(from_wallet_address, nullptr, true, false);
  std::vector<mojom::SolanaAccountMetaPtr> account_metas;
  account_metas.push_back(std::move(solana_account_meta1));
  account_metas.push_back(mint_account_meta.Clone());
  account_metas.push_back(std::move(solana_account_meta2));
  account_metas.push_back(std::move(solana_account_meta3));

  auto mojom_param = mojom::SolanaInstructionParam::New(
      "amount", "Amount", "10000000",
      mojom::SolanaInstructionParamType::kUint64);
  auto mojom_decimals_param = mojom::SolanaInstructionParam::New(
      "decimals", "Decimals", "8", mojom::SolanaInstructionParamType::kUint8);
  std::vector<mojom::SolanaInstructionParamPtr> mojom_params;
  mojom_params.emplace_back(std::move(mojom_param));
  mojom_params.emplace_back(std::move(mojom_decimals_param));
  auto mojom_decoded_data = mojom::DecodedSolanaInstructionData::New(
      static_cast<uint32_t>(mojom::SolanaTokenInstruction::kTransferChecked),
      solana_ins_data_decoder::GetMojomAccountParamsForTesting(
          std::nullopt, mojom::SolanaTokenInstruction::kTransferChecked),
      std::move(mojom_params));

  auto mojom_transfer_instruction =
      mojom::SolanaInstruction::New(token_program_id, std::move(account_metas),
                                    data, std::move(mojom_decoded_data));

  std::vector<mojom::SolanaInstructionPtr> instructions;
  instructions.push_back(mojom_transfer_instruction.Clone());
  auto tx_data = mojom::SolanaTxData::New(
      "", 0, from_wallet_address, to_wallet_address, spl_token_mint_address, 0,
      10000000, mojom::TransactionType::SolanaSPLTokenTransfer,
      std::move(instructions), mojom::SolanaMessageVersion::kLegacy,
      mojom::SolanaMessageHeader::New(1, 0, 2),
      std::vector<std::string>({from_wallet_address,
                                *from_associated_token_account,
                                *to_associated_token_account, token_program_id,
                                spl_token_mint_address}),
      std::vector<mojom::SolanaMessageAddressTableLookupPtr>(), nullptr,
      nullptr, nullptr);

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
          "owner":"$1",
          "rentEpoch":284
        }
      }
    }
  )";

  SetInterceptor(
      "", 0, "",
      base::ReplaceStringPlaceholders(json, {token_program_id}, nullptr));
  TestMakeTokenProgramTransferTxData(
      FROM_HERE, mojom::kSolanaMainnet, spl_token_mint_address,
      from_wallet_address, to_wallet_address, 10000000, std::move(tx_data),
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
  auto solana_account_meta9 =
      mojom::SolanaAccountMeta::New(token_program_id, nullptr, false, false);
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
           token_program_id}),
      std::vector<mojom::SolanaMessageAddressTableLookupPtr>(), nullptr,
      nullptr, nullptr);

  // Test owner is not token program account.
  SetAccountInfoInterceptor(
      {{spl_token_mint_address, token_program_id},
       {*to_associated_token_account, "11111111111111111111111111111111"}});
  TestMakeTokenProgramTransferTxData(
      FROM_HERE, mojom::kSolanaMainnet, spl_token_mint_address,
      from_wallet_address, to_wallet_address, 10000000, tx_data.Clone(),
      mojom::SolanaProviderError::kSuccess, "");

  // Test receiving associated token account does not exist.
  SetAccountInfoInterceptor({{spl_token_mint_address, token_program_id},
                             {*to_associated_token_account, ""}});
  TestMakeTokenProgramTransferTxData(
      FROM_HERE, mojom::kSolanaMainnet, spl_token_mint_address,
      from_wallet_address, to_wallet_address, 10000000, std::move(tx_data),
      mojom::SolanaProviderError::kSuccess, "");

  // Empty addresses should be handled.
  TestMakeTokenProgramTransferTxData(
      FROM_HERE, mojom::kSolanaMainnet, "", to_wallet_address,
      spl_token_mint_address, 10000000, nullptr,
      mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  TestMakeTokenProgramTransferTxData(
      FROM_HERE, mojom::kSolanaMainnet, from_wallet_address, "",
      spl_token_mint_address, 10000000, nullptr,
      mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  TestMakeTokenProgramTransferTxData(
      FROM_HERE, mojom::kSolanaMainnet, from_wallet_address, to_wallet_address,
      "", 10000000, nullptr, mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Test sending to OFAC Sanctioned address
  const std::string ofac_sanctioned_to =
      "FepMPR8vahkJ98Fr22VKbfHU4f4PTAyi18PDZN2NooPb";
  auto* registry = BlockchainRegistry::GetInstance();
  registry->UpdateOfacAddressesList({base::ToLowerASCII(ofac_sanctioned_to)});
  TestMakeTokenProgramTransferTxData(
      FROM_HERE, mojom::kSolanaMainnet, spl_token_mint_address,
      from_wallet_address, ofac_sanctioned_to, 10000000, std::move(tx_data),
      mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_OFAC_RESTRICTION));
}

INSTANTIATE_TEST_SUITE_P(TokenProgramTest,
                         TokenProgramTest,
                         testing::Values(mojom::SPLTokenProgram::kToken,
                                         mojom::SPLTokenProgram::kToken2022));

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
      send_options.ToMojomSendOptions(), nullptr, nullptr);
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

TEST_F(SolanaTxManagerUnitTest, DropTxWithInvalidBlockhash) {
  const auto& from = sol_account();
  const std::string to = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  mojom::SolanaTxDataPtr system_transfer_data = nullptr;
  TestMakeSystemProgramTransferTxData(from, to, 10000000, nullptr,
                                      mojom::SolanaProviderError::kSuccess, "",
                                      &system_transfer_data);
  ASSERT_TRUE(system_transfer_data);

  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_, "",
                 false, last_valid_block_height1_);
  std::string meta_id1;
  AddUnapprovedTransaction(mojom::kSolanaMainnet, system_transfer_data.Clone(),
                           from, &meta_id1);
  ASSERT_FALSE(meta_id1.empty());
  ApproveTransaction(meta_id1);

  SetInterceptor(latest_blockhash2_, last_valid_block_height2_, tx_hash2_, "",
                 false, last_valid_block_height1_);
  // Fast forward to have block tracker run with the new interceptor.
  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));

  std::string meta_id2;
  AddUnapprovedTransaction(mojom::kSolanaMainnet, system_transfer_data.Clone(),
                           from, &meta_id2);
  ASSERT_FALSE(meta_id2.empty());
  ApproveTransaction(meta_id2);

  WaitForUpdatePendingTransactions();

  // Check two submitted tx.
  auto pending_transactions =
      solana_tx_manager()->GetSolanaTxStateManager()->GetTransactionsByStatus(
          mojom::kSolanaMainnet, mojom::TransactionStatus::Submitted,
          std::nullopt);
  EXPECT_EQ(pending_transactions.size(), 2u);
  auto tx1 = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx1);
  EXPECT_EQ(tx1->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx1->tx()->message()->recent_blockhash(), latest_blockhash1_);
  EXPECT_EQ(tx1->tx()->message()->last_valid_block_height(),
            last_valid_block_height1_);

  auto tx2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx2);
  EXPECT_EQ(tx2->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx2->tx()->message()->recent_blockhash(), latest_blockhash2_);
  EXPECT_EQ(tx2->tx()->message()->last_valid_block_height(),
            last_valid_block_height2_);

  // Set Interceptor for return null signature statuses and block height only
  // valid for blockhash2.
  SetInterceptor(latest_blockhash3_, last_valid_block_height3_, "", "", true,
                 last_valid_block_height2_, "[null, null]");
  // Fast forward to have block tracker run with the new interceptor.
  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));
  WaitForUpdatePendingTransactions();

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

TEST_F(SolanaTxManagerUnitTest, DropTxWithInvalidBlockhash_DappBlockhash) {
  mojom::SolanaTxDataPtr system_transfer_data =
      GetSystemProgramTransferTxData(sol_account(), to_account());
  ASSERT_TRUE(system_transfer_data);
  system_transfer_data->recent_blockhash = latest_blockhash1_;
  ASSERT_EQ(system_transfer_data->last_valid_block_height, 0u);

  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_);
  std::string meta_id1;
  AddUnapprovedTransaction(mojom::kSolanaMainnet, system_transfer_data.Clone(),
                           sol_account(), &meta_id1);
  ASSERT_FALSE(meta_id1.empty());
  ApproveTransaction(meta_id1);

  std::string meta_id2;
  auto system_transfer_data2 = system_transfer_data.Clone();
  system_transfer_data2->recent_blockhash = latest_blockhash2_;
  system_transfer_data2->last_valid_block_height = 0;
  AddUnapprovedTransaction(mojom::kSolanaMainnet,
                           std::move(system_transfer_data2), sol_account(),
                           &meta_id2);
  ASSERT_FALSE(meta_id2.empty());
  ApproveTransaction(meta_id2);

  auto tx_meta2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  ASSERT_EQ(tx_meta2->tx()->message()->last_valid_block_height(),
            last_valid_block_height1_ + kSolanaValidBlockHeightThreshold);
  tx_meta2->tx()->message()->set_last_valid_block_height(
      last_valid_block_height2_ + kSolanaValidBlockHeightThreshold);
  ASSERT_TRUE(
      solana_tx_manager()->GetSolanaTxStateManager()->AddOrUpdateTx(*tx_meta2));

  WaitForUpdatePendingTransactions();

  // Last valid block height should be set during ApproveTransaction.
  auto tx_meta = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta);
  ASSERT_EQ(tx_meta->tx()->message()->recent_blockhash(), latest_blockhash1_);
  ASSERT_EQ(tx_meta->tx()->message()->last_valid_block_height(),
            last_valid_block_height1_ + kSolanaValidBlockHeightThreshold);
  EXPECT_EQ(tx_meta->status(), mojom::TransactionStatus::Submitted);

  tx_meta2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  ASSERT_EQ(tx_meta2->tx()->message()->recent_blockhash(), latest_blockhash2_);
  ASSERT_EQ(tx_meta2->tx()->message()->last_valid_block_height(),
            last_valid_block_height2_ + 150);
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Submitted);

  ASSERT_TRUE(solana_tx_manager()->GetSolanaBlockTracker()->IsRunning(
      mojom::kSolanaMainnet));

  // Trigger block tracker to run, with block_height < last_valid_block_height.
  // Transaction stay as submitted.
  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));
  WaitForUpdatePendingTransactions();

  tx_meta = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->status(), mojom::TransactionStatus::Submitted);

  tx_meta2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Submitted);

  ASSERT_TRUE(solana_tx_manager()->GetSolanaBlockTracker()->IsRunning(
      mojom::kSolanaMainnet));

  // Fast forward to have block tracker run with the new interceptor, where
  // block_height > last_valid_block_height.
  // Transaction should be dropped.
  SetInterceptor(latest_blockhash2_, last_valid_block_height2_, "", "", true,
                 last_valid_block_height1_ + 151, "[null, null]");
  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));
  WaitForUpdatePendingTransactions();

  tx_meta = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->status(), mojom::TransactionStatus::Dropped);

  tx_meta2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Submitted);
}

TEST_F(SolanaTxManagerUnitTest, DropTxAfterSafeDropThreshold) {
  const auto& from = sol_account();
  const std::string to = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  mojom::SolanaTxDataPtr system_transfer_data = nullptr;
  TestMakeSystemProgramTransferTxData(from, to, 10000000, nullptr,
                                      mojom::SolanaProviderError::kSuccess, "",
                                      &system_transfer_data);
  ASSERT_TRUE(system_transfer_data);

  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, true, last_valid_block_height1_,
                 "[null]");
  std::string meta_id1;
  AddUnapprovedTransaction(mojom::kSolanaMainnet, system_transfer_data.Clone(),
                           from, &meta_id1);
  ASSERT_FALSE(meta_id1.empty());

  send_transaction_calls_ = 0;
  ApproveTransaction(meta_id1);
  WaitForUpdatePendingTransactions();
  EXPECT_EQ(send_transaction_calls_, 1u);

  auto tx = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx);
  tx->set_submitted_time(base::Time::Now() - base::Minutes(30));
  ASSERT_TRUE(
      solana_tx_manager()->GetSolanaTxStateManager()->AddOrUpdateTx(*tx));

  send_transaction_calls_ = 0;
  SetInterceptor(latest_blockhash2_, last_valid_block_height2_, tx_hash1_, "",
                 true, last_valid_block_height1_, "[null]");
  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));
  WaitForUpdatePendingTransactions();

  EXPECT_EQ(send_transaction_calls_, 0u);
  tx = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx);
  EXPECT_EQ(tx->status(), mojom::TransactionStatus::Dropped);
}

TEST_F(SolanaTxManagerUnitTest, RetryTransaction) {
  // Add a transaction with blockhash.
  mojom::SolanaTxDataPtr tx_data =
      GetSystemProgramTransferTxData(sol_account(), to_account());
  ASSERT_TRUE(tx_data);
  tx_data->recent_blockhash = latest_blockhash1_;
  tx_data->last_valid_block_height = last_valid_block_height1_;
  std::string meta_id1;
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_);
  AddUnapprovedTransaction(mojom::kSolanaMainnet, std::move(tx_data),
                           sol_account(), &meta_id1);

  // Add a transaction with durable nonce.
  mojom::SolanaTxDataPtr durable_nonce_tx_data =
      GetSystemProgramTransferTxData(sol_account(), to_account());
  auto nonce_account = SolAccount(3);
  durable_nonce_tx_data->recent_blockhash = nonce_account->address;
  durable_nonce_tx_data->last_valid_block_height =
      last_valid_block_height1_ + kSolanaValidBlockHeightThreshold;
  std::string meta_id2;
  AddUnapprovedTransaction(mojom::kSolanaMainnet,
                           std::move(durable_nonce_tx_data), sol_account(),
                           &meta_id2);

  auto durable_nonce_meta = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(durable_nonce_meta);

  // Put AdvanceNonceAccount instruction before the transfer instruction.
  SolanaInstruction instruction = GetAdvanceNonceAccountInstruction();
  std::vector<SolanaInstruction> vec;
  vec.emplace_back(instruction);
  vec.emplace_back(durable_nonce_meta->tx()->message()->instructions()[0]);
  durable_nonce_meta->tx()->message()->SetInstructionsForTesting(vec);

  ASSERT_TRUE(solana_tx_manager()->GetSolanaTxStateManager()->AddOrUpdateTx(
      *durable_nonce_meta));

  // Test retry transaction with invalid state.
  TestRetryTransaction(
      FROM_HERE, meta_id1, false,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_RETRIABLE));

  // Update both transactions to dropped.
  auto tx_meta1 = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  auto tx_meta2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  tx_meta1->set_status(mojom::TransactionStatus::Dropped);
  tx_meta2->set_status(mojom::TransactionStatus::Dropped);
  ASSERT_TRUE(
      solana_tx_manager()->GetSolanaTxStateManager()->AddOrUpdateTx(*tx_meta1));
  ASSERT_TRUE(
      solana_tx_manager()->GetSolanaTxStateManager()->AddOrUpdateTx(*tx_meta2));

  TestRetryTransaction(FROM_HERE, meta_id1, true, "", "");
  TestRetryTransaction(FROM_HERE, meta_id2, true, "", nonce_account->address);

  // Test retry partial signed transaction, only the one using durable nonce
  // can be retried.
  tx_meta1 = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  tx_meta2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);

  auto param = mojom::SolanaSignTransactionParam::New(
      "test", std::vector<mojom::SignaturePubkeyPairPtr>());
  param->signatures.emplace_back(mojom::SignaturePubkeyPair::New(
      std::vector<uint8_t>(kSolanaSignatureSize, 1), sol_account()->address));
  tx_meta1->tx()->set_sign_tx_param(param.Clone());
  tx_meta2->tx()->set_sign_tx_param(param.Clone());

  ASSERT_TRUE(
      solana_tx_manager()->GetSolanaTxStateManager()->AddOrUpdateTx(*tx_meta1));
  ASSERT_TRUE(
      solana_tx_manager()->GetSolanaTxStateManager()->AddOrUpdateTx(*tx_meta2));
  TestRetryTransaction(
      FROM_HERE, meta_id1, false,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_RETRIABLE));
  TestRetryTransaction(FROM_HERE, meta_id2, true, "", nonce_account->address);

  // Test retry transaction with unknown tx id.
  TestRetryTransaction(
      FROM_HERE, "UnknownTxID", false,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));

  // Test retry transaction with invalid tx type.
  tx_meta1->tx()->set_tx_type(mojom::TransactionType::SolanaSwap);
  ASSERT_TRUE(
      solana_tx_manager()->GetSolanaTxStateManager()->AddOrUpdateTx(*tx_meta1));
  TestRetryTransaction(
      FROM_HERE, meta_id1, false,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_RETRIABLE));
}

TEST_F(SolanaTxManagerUnitTest, GetTransactionMessageToSign) {
  // Unknown tx_meta_id yields null message
  TestGetTransactionMessageToSign("Unknown", std::nullopt);
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
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_);
  std::string system_transfer_meta_id;
  AddUnapprovedTransaction(mojom::kSolanaMainnet,
                           std::move(system_transfer_data), from,
                           &system_transfer_meta_id);
  ASSERT_FALSE(system_transfer_meta_id.empty());

  // Valid latest blockhash yields valid transaction message to sign
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, "");
  std::optional<std::vector<std::uint8_t>> message = base::Base64Decode(
      "AQACBGodJRVUDnxVZv71pBNy0DZ/"
      "ui6dv1N37VgGEA+"
      "aezhZAMzywrLOSju1o9VJQ5KaB2lsblgqvdjtkDFlmZHz4KQDBkZv5SEXMv/"
      "srbpyw5vnvIzlu8X3EmssQ5s6QAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AzEkOkozS44c7s0P8ldozF5ymD02/"
      "RsLDbpEpnVXU5rkDAgAFAsUOAQACAAkDZAAAAAAAAAADAgABDAIAAAABAAAAAAAAAA==");
  TestGetTransactionMessageToSign(system_transfer_meta_id, message);

  // Valid cached latest blockhash
  SetInterceptor("", 0, "", "");
  TestGetTransactionMessageToSign(system_transfer_meta_id, message);
}

TEST_F(SolanaTxManagerUnitTest, ProcessSolanaHardwareSignature) {
  // Unknown tx_meta_id is invalid
  TestProcessSolanaHardwareSignature(
      "Unknown", std::vector<uint8_t>(), false,
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
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_);
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

  auto meta = solana_tx_manager()->GetTxForTesting(system_transfer_meta_id);
  meta->tx()->message()->set_recent_blockhash(latest_blockhash1_);
  meta->tx()->message()->set_last_valid_block_height(last_valid_block_height1_);
  ASSERT_TRUE(
      solana_tx_manager()->GetSolanaTxStateManager()->AddOrUpdateTx(*meta));

  // Valid blockhash and valid number of signers is valid
  TestProcessSolanaHardwareSignature(system_transfer_meta_id, signature_bytes,
                                     true, mojom::SolanaProviderError::kSuccess,
                                     "");
}

TEST_F(SolanaTxManagerUnitTest, RebroadcastTransaction) {
  auto data = GetSystemProgramTransferTxData(sol_account(), to_account());
  ASSERT_TRUE(data);
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, "", "", true,
                 last_valid_block_height1_, "[null]");

  std::string meta_id1;
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_);
  AddUnapprovedTransaction(mojom::kSolanaMainnet, data.Clone(), sol_account(),
                           &meta_id1);
  ASSERT_FALSE(meta_id1.empty());

  // No rebroadcast when first sending out the tx.
  ApproveTransaction(meta_id1);
  WaitForUpdatePendingTransactions();
  EXPECT_EQ(send_transaction_calls_, 1u);

  // Rebradcast should happen once for tx with null signature status.
  send_transaction_calls_ = 0;
  SetInterceptor(latest_blockhash2_, last_valid_block_height2_, "", "", true,
                 last_valid_block_height1_, "[null]");
  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));
  WaitForUpdatePendingTransactions();
  EXPECT_EQ(send_transaction_calls_, 1u);

  // Rebroadcast should happen once when tx is not confirmed and latest
  // blockhash is updated which triggers UpdatePendingTransaction.
  send_transaction_calls_ = 0;
  SetInterceptor(latest_blockhash3_, last_valid_block_height3_, "", "", true,
                 last_valid_block_height1_,
                 R"([{"slot": 100, "confirmations": 100, "err": null,
                      "confirmationStatus": "processed"}])");

  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));
  WaitForUpdatePendingTransactions();
  EXPECT_EQ(send_transaction_calls_, 1u);

  // No rebroadcast when blockhash is expired.
  send_transaction_calls_ = 0;
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, "",
                 kMockGetFeeForMessageResponse, true, last_valid_block_height2_,
                 R"([{"slot": 100, "confirmations": 100, "err": null,
                      "confirmationStatus": "processed"}])");
  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));
  WaitForUpdatePendingTransactions();
  EXPECT_EQ(send_transaction_calls_, 0u);

  SetInterceptor(latest_blockhash2_, last_valid_block_height2_, "",
                 kMockGetFeeForMessageResponse, true, last_valid_block_height2_,
                 "[null, null]");
  data->send_options = mojom::SolanaSendTransactionOptions::New(
      mojom::OptionalMaxRetries::New(1u), std::nullopt, nullptr);
  std::string meta_id2;
  AddUnapprovedTransaction(mojom::kSolanaMainnet, data.Clone(), sol_account(),
                           &meta_id2);
  ASSERT_FALSE(meta_id2.empty());

  // Avoid cached blockhash being used when approve.
  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));
  WaitForUpdatePendingTransactions();

  // No rebroadcast when first sending out the tx with customized MaxRetries.
  send_transaction_calls_ = 0;
  ApproveTransaction(meta_id2);
  WaitForUpdatePendingTransactions();
  EXPECT_EQ(send_transaction_calls_, 1u);

  // Rebroadcast when tx signature status is null with customized MaxRetries.
  SetInterceptor(latest_blockhash3_, last_valid_block_height3_, "", "", true,
                 last_valid_block_height2_, "[null]");
  send_transaction_calls_ = 0;
  task_environment_.FastForwardBy(
      base::Seconds(kSolanaBlockTrackerTimeInSeconds));
  WaitForUpdatePendingTransactions();
  EXPECT_EQ(send_transaction_calls_, 1u);
}

TEST_F(SolanaTxManagerUnitTest, GetSolanaTxFeeEstimation) {
  // Fetching fee estimate for non existant tx id meta fails.
  TestGetSolanaTxFeeEstimation(
      mojom::kSolanaMainnet, "non existant tx meta id", {},
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));

  // Add an unapproved tx manually (circumventing the fee estimation fetching
  // built into that function).
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
      nullptr, nullptr);

  auto tx = SolanaTransaction::FromSolanaTxData(solana_tx_data.Clone());
  ASSERT_TRUE(tx);

  SolanaTxMeta meta(from_account, std::move(tx));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_created_time(base::Time::Now());
  meta.set_status(mojom::TransactionStatus::Unapproved);
  meta.set_chain_id(mojom::kSolanaMainnet);

  ASSERT_TRUE(
      solana_tx_manager()->GetSolanaTxStateManager()->AddOrUpdateTx(meta));
  task_environment_.RunUntilIdle();

  // Call fetch fee estimation with appropriate interceptors. Verify median and
  // priority fees.
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_,
                 kMockGetFeeForMessageResponse, false,
                 last_valid_block_height1_);
  mojom::SolanaFeeEstimationPtr expected_estimate =
      mojom::SolanaFeeEstimation::New();
  expected_estimate->base_fee = 5000;
  expected_estimate->compute_units = 69017 + 300;
  expected_estimate->fee_per_compute_unit = 100;
  TestGetSolanaTxFeeEstimation(mojom::kSolanaMainnet, meta.id(),
                               std::move(expected_estimate),
                               mojom::SolanaProviderError::kSuccess, "");
}

TEST_F(SolanaTxManagerUnitTest, DecodeMerkleTreeAuthorityAndDepth) {
  const auto merkle_tree_account_info = base::Base64Decode(
      R"(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)");

  ASSERT_TRUE(merkle_tree_account_info);
  auto result = solana_tx_manager()->DecodeMerkleTreeAuthorityAndDepth(
      *merkle_tree_account_info);

  ASSERT_TRUE(result);
  EXPECT_EQ((*result).second.ToBase58(),
            "2rm66D8wbJEfb9vNDuwmk5UhLj18h9ZURxVSRM13gzNL");
  EXPECT_EQ((*result).first, 0u);
}

}  // namespace brave_wallet
