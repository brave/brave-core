/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_manager.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/solana_block_tracker.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/solana_tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

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
    last_valid_block_height1_ = 3090;
    last_valid_block_height2_ = 3290;

    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletSolanaFeature);

    SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash1_);
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_.reset(
        new JsonRpcService(shared_url_loader_factory_, &prefs_));
    keyring_service_.reset(
        new KeyringService(json_rpc_service_.get(), &prefs_));
    tx_service_.reset(new TxService(json_rpc_service_.get(),
                                    keyring_service_.get(), &prefs_));
    CreateWallet();
    AddAccount();
  }

  void CreateWallet() {
    base::RunLoop run_loop;
    keyring_service_->CreateWallet(
        "brave",
        base::BindLambdaForTesting([&run_loop](const std::string& mnemonic) {
          ASSERT_FALSE(mnemonic.empty());
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void AddAccount() {
    base::RunLoop run_loop;
    keyring_service_->AddAccount(
        "New Account", mojom::CoinType::SOL,
        base::BindLambdaForTesting([&run_loop](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void SetInterceptor(const std::string& latest_blockhash,
                      uint64_t last_valid_block_height,
                      const std::string& tx_hash,
                      const std::string& content = "",
                      bool get_signature_statuses = false) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, latest_blockhash, tx_hash, content, get_signature_statuses,
         last_valid_block_height](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          base::StringPiece request_string(request.request_body->elements()
                                               ->at(0)
                                               .As<network::DataElementBytes>()
                                               .AsStringPiece());
          absl::optional<base::Value> request_value =
              base::JSONReader::Read(request_string);
          std::string* method = request_value->FindStringKey("method");
          ASSERT_TRUE(method);

          if (*method == "getLatestBlockhash") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "{\"context\":{\"slot\":1069},\"value\":{\"blockhash\":\"" +
                    latest_blockhash + "\", \"lastValidBlockHeight\":" +
                    std::to_string(last_valid_block_height) + "}}}");
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
            const base::Value* params_list =
                request_value->FindListKey("params");
            ASSERT_TRUE(params_list && params_list->GetList()[0].is_list());
            const std::string* hash =
                params_list->GetList()[0].GetList()[0].GetIfString();
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

  void AddUnapprovedTransaction(mojom::SolanaTxDataPtr solana_tx_data,
                                const std::string& from,
                                std::string* meta_id) {
    auto tx_data_union =
        mojom::TxDataUnion::NewSolanaTxData(std::move(solana_tx_data));

    base::RunLoop run_loop;
    solana_tx_manager()->AddUnapprovedTransaction(
        std::move(tx_data_union), from,
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

  void TestMakeTokenProgramTransferTxData(
      const std::string& spl_token_mint_address,
      const std::string& from_wallet_address,
      const std::string& to_wallet_address,
      uint64_t amount,
      mojom::SolanaTxDataPtr expected_tx_data,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_err_message) {
    base::RunLoop run_loop;
    solana_tx_manager()->MakeTokenProgramTransferTxData(
        spl_token_mint_address, from_wallet_address, to_wallet_address, amount,
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

  void TestGetEstimatedTxFee(const std::string& tx_meta_id,
                             uint64_t expected_tx_fee,
                             mojom::SolanaProviderError expected_error,
                             const std::string& expected_err_message) {
    base::RunLoop run_loop;
    solana_tx_manager()->GetEstimatedTxFee(
        tx_meta_id, base::BindLambdaForTesting(
                        [&](uint64_t tx_fee, mojom::SolanaProviderError error,
                            const std::string& err_message) {
                          EXPECT_EQ(expected_tx_fee, tx_fee);
                          EXPECT_EQ(expected_error, error);
                          EXPECT_EQ(expected_err_message, err_message);
                          run_loop.Quit();
                        }));
    run_loop.Run();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TxService> tx_service_;
  std::string tx_hash1_;
  std::string tx_hash2_;
  std::string latest_blockhash1_;
  std::string latest_blockhash2_;
  uint64_t last_valid_block_height1_ = 0;
  uint64_t last_valid_block_height2_ = 0;
};

TEST_F(SolanaTxManagerUnitTest, AddAndApproveTransaction) {
  // Stop the block tracker explicitly to make sure it will be started when we
  // submit our pending transactions.
  solana_tx_manager()->GetSolanaBlockTracker()->Stop();

  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  std::vector<mojom::SolanaAccountMetaPtr> account_metas;
  auto account_meta1 = mojom::SolanaAccountMeta::New(from_account, true, true);
  auto account_meta2 = mojom::SolanaAccountMeta::New(to_account, false, true);
  account_metas.push_back(std::move(account_meta1));
  account_metas.push_back(std::move(account_meta2));

  auto instruction = mojom::SolanaInstruction::New(
      kSolanaSystemProgramId, std::move(account_metas), data);
  std::vector<mojom::SolanaInstructionPtr> instructions;
  instructions.push_back(std::move(instruction));

  auto solana_tx_data = mojom::SolanaTxData::New(
      "" /* recent_blockhash */, 0, from_account, to_account,
      "" /* spl_token_mint_address */, 10000000u /* lamport */, 0 /* amount */,
      mojom::TransactionType::SolanaSystemTransfer, std::move(instructions));

  auto tx = SolanaTransaction::FromSolanaTxData(solana_tx_data.Clone());
  ASSERT_TRUE(tx);

  std::string meta_id1;
  AddUnapprovedTransaction(solana_tx_data.Clone(), from_account, &meta_id1);

  auto tx_meta1 = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_EQ(*tx_meta1->tx(), *tx);
  EXPECT_EQ(tx_meta1->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Unapproved);

  std::string meta_id2;
  AddUnapprovedTransaction(solana_tx_data.Clone(), from_account, &meta_id2);
  auto tx_meta2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_EQ(*tx_meta2->tx(), *tx);
  EXPECT_EQ(tx_meta2->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta2->from(), from_account);
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Unapproved);

  ApproveTransaction(meta_id1);
  // Wait for tx to be updated.
  base::RunLoop().RunUntilIdle();

  auto approved_tx = std::make_unique<SolanaTransaction>(*tx);
  approved_tx->message()->set_recent_blockhash(latest_blockhash1_);
  approved_tx->message()->set_last_valid_block_height(
      last_valid_block_height1_);

  tx_meta1 = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_EQ(*tx_meta1->tx(), *approved_tx);
  EXPECT_EQ(tx_meta1->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx_meta1->tx_hash(), tx_hash1_);

  // Send another tx.
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, tx_hash2_);
  ApproveTransaction(meta_id2);
  base::RunLoop().RunUntilIdle();

  tx_meta2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_EQ(*tx_meta2->tx(), *approved_tx);
  EXPECT_EQ(tx_meta2->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta2->from(), from_account);
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx_meta2->tx_hash(), tx_hash2_);

  // Fast forward to have block tracker run with current interceptor.
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));

  SetInterceptor(latest_blockhash2_, last_valid_block_height2_, tx_hash2_, "",
                 true);

  // Fast forward again to have block tracker run with the new interceptor.
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));
  tx_meta1 = solana_tx_manager()->GetTxForTesting(meta_id1);
  EXPECT_EQ(mojom::TransactionStatus::Submitted, tx_meta1->status());
  EXPECT_EQ(tx_meta1->signature_status(),
            SolanaSignatureStatus(100u, 10u, "", "confirmed"));

  tx_meta2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  EXPECT_EQ(mojom::TransactionStatus::Confirmed, tx_meta2->status());
  EXPECT_EQ(tx_meta2->signature_status(),
            SolanaSignatureStatus(72u, 0u, "", "finalized"));
}

TEST_F(SolanaTxManagerUnitTest, MakeSystemProgramTransferTxData) {
  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  auto solana_account_meta1 =
      mojom::SolanaAccountMeta::New(from_account, true, true);
  auto solana_account_meta2 =
      mojom::SolanaAccountMeta::New(to_account, false, true);
  std::vector<mojom::SolanaAccountMetaPtr> account_metas;
  account_metas.push_back(std::move(solana_account_meta1));
  account_metas.push_back(std::move(solana_account_meta2));
  auto mojom_instruction = mojom::SolanaInstruction::New(
      kSolanaSystemProgramId, std::move(account_metas), data);

  std::vector<mojom::SolanaInstructionPtr> instructions;
  instructions.push_back(std::move(mojom_instruction));
  auto tx_data = mojom::SolanaTxData::New(
      "", 0, from_account, to_account, "", 10000000, 0,
      mojom::TransactionType::SolanaSystemTransfer, std::move(instructions));

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
      *from_associated_token_account, false, true);
  auto solana_account_meta2 =
      mojom::SolanaAccountMeta::New(*to_associated_token_account, false, true);
  auto solana_account_meta3 =
      mojom::SolanaAccountMeta::New(from_wallet_address, true, false);
  std::vector<mojom::SolanaAccountMetaPtr> account_metas;
  account_metas.push_back(std::move(solana_account_meta1));
  account_metas.push_back(std::move(solana_account_meta2));
  account_metas.push_back(std::move(solana_account_meta3));
  auto mojom_transfer_instruction = mojom::SolanaInstruction::New(
      kSolanaTokenProgramId, std::move(account_metas), data);

  std::vector<mojom::SolanaInstructionPtr> instructions;
  instructions.push_back(mojom_transfer_instruction.Clone());
  auto tx_data = mojom::SolanaTxData::New(
      "", 0, from_wallet_address, to_wallet_address, spl_token_mint_address, 0,
      10000000, mojom::TransactionType::SolanaSPLTokenTransfer,
      std::move(instructions));

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
      spl_token_mint_address, from_wallet_address, to_wallet_address, 10000000,
      std::move(tx_data), mojom::SolanaProviderError::kSuccess, "");

  auto solana_account_meta4 =
      mojom::SolanaAccountMeta::New(from_wallet_address, true, true);
  auto solana_account_meta5 =
      mojom::SolanaAccountMeta::New(*to_associated_token_account, false, true);
  auto solana_account_meta6 =
      mojom::SolanaAccountMeta::New(to_wallet_address, false, false);
  auto solana_account_meta7 =
      mojom::SolanaAccountMeta::New(spl_token_mint_address, false, false);
  auto solana_account_meta8 =
      mojom::SolanaAccountMeta::New(kSolanaSystemProgramId, false, false);
  auto solana_account_meta9 =
      mojom::SolanaAccountMeta::New(kSolanaTokenProgramId, false, false);
  auto solana_account_meta10 =
      mojom::SolanaAccountMeta::New(kSolanaSysvarRentProgramId, false, false);
  account_metas.push_back(std::move(solana_account_meta4));
  account_metas.push_back(std::move(solana_account_meta5));
  account_metas.push_back(std::move(solana_account_meta6));
  account_metas.push_back(std::move(solana_account_meta7));
  account_metas.push_back(std::move(solana_account_meta8));
  account_metas.push_back(std::move(solana_account_meta9));
  account_metas.push_back(std::move(solana_account_meta10));
  auto mojom_create_associated_account_instruction =
      mojom::SolanaInstruction::New(kSolanaAssociatedTokenProgramId,
                                    std::move(account_metas),
                                    std::vector<uint8_t>());
  instructions.push_back(
      std::move(mojom_create_associated_account_instruction));
  instructions.push_back(std::move(mojom_transfer_instruction));
  tx_data = mojom::SolanaTxData::New(
      "", 0, from_wallet_address, to_wallet_address, spl_token_mint_address, 0,
      10000000,
      mojom::TransactionType::
          SolanaSPLTokenTransferWithAssociatedTokenAccountCreation,
      std::move(instructions));

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
      spl_token_mint_address, from_wallet_address, to_wallet_address, 10000000,
      tx_data.Clone(), mojom::SolanaProviderError::kSuccess, "");

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
      spl_token_mint_address, from_wallet_address, to_wallet_address, 10000000,
      std::move(tx_data), mojom::SolanaProviderError::kSuccess, "");

  // Empty addresses should be handled.
  TestMakeTokenProgramTransferTxData(
      "", to_wallet_address, spl_token_mint_address, 10000000, nullptr,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  TestMakeTokenProgramTransferTxData(
      from_wallet_address, "", spl_token_mint_address, 10000000, nullptr,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  TestMakeTokenProgramTransferTxData(
      from_wallet_address, to_wallet_address, "", 10000000, nullptr,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(SolanaTxManagerUnitTest, GetEstimatedTxFee) {
  const std::string from = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  const std::string to = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  mojom::SolanaTxDataPtr system_transfer_data = nullptr;
  TestMakeSystemProgramTransferTxData(from, to, 10000000, nullptr,
                                      mojom::SolanaProviderError::kSuccess, "",
                                      &system_transfer_data);
  ASSERT_TRUE(system_transfer_data);

  std::string system_transfer_meta_id;
  AddUnapprovedTransaction(std::move(system_transfer_data), from,
                           &system_transfer_meta_id);
  ASSERT_FALSE(system_transfer_meta_id.empty());

  std::string json = R"({
      "jsonrpc": "2.0",
      "result": { "context": { "slot": 5068 }, "value": 12345 },
      "id": 1
  })";

  // GetEstimatedTxFee without a valid latest blockhash being returned by
  // remote.
  SetInterceptor("", 0, "", json);
  TestGetEstimatedTxFee(system_transfer_meta_id, 0,
                        mojom::SolanaProviderError::kParsingError,
                        l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  std::string json2 = R"({
      "jsonrpc": "2.0",
      "result": { "context": { "slot": 5068 }, "value": null },
      "id": 1
  })";

  // GetEstimatedTxFee with latest blockhash and non-null tx fee from remote.
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, "", json);
  TestGetEstimatedTxFee(system_transfer_meta_id, 12345,
                        mojom::SolanaProviderError::kSuccess, "");

  // GetEstimatedTxFee with cached blockhash and error at parsing tx fee.
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, "", "");
  TestGetEstimatedTxFee(system_transfer_meta_id, 0,
                        mojom::SolanaProviderError::kParsingError,
                        l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // GetEstimatedTxFee with cached blockhash and null tx fee from remote.
  SetInterceptor(latest_blockhash1_, last_valid_block_height1_, "", json2);
  TestGetEstimatedTxFee(system_transfer_meta_id, 0,
                        mojom::SolanaProviderError::kSuccess, "");

  // GetEstimatedTxFee with cached latest blockhash and non-null tx fee from
  // remote.
  SetInterceptor("", 0, "", json);
  TestGetEstimatedTxFee(system_transfer_meta_id, 12345,
                        mojom::SolanaProviderError::kSuccess, "");
}

}  // namespace brave_wallet
