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
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/solana_tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

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

    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletSolanaFeature);

    SetInterceptor(latest_blockhash1_, tx_hash1_);
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_.reset(
        new JsonRpcService(shared_url_loader_factory_, &prefs_));
    keyring_service_.reset(new KeyringService(&prefs_));
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
                      const std::string& tx_hash,
                      bool get_signature_statuses = false) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, get_signature_statuses](const network::ResourceRequest& request) {
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
                    latest_blockhash + "\", \"lastValidBlockHeight\":3090}}}");
          } else if (*method == "sendTransaction") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" + tx_hash +
                    "\"}");
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

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TxService> tx_service_;
  std::string tx_hash1_;
  std::string tx_hash2_;
  std::string latest_blockhash1_;
  std::string latest_blockhash2_;
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

  auto solana_tx_data =
      mojom::SolanaTxData::New("", from_account, std::move(instructions));

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

  tx_meta1 = solana_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_EQ(*tx_meta1->tx(), *tx);
  EXPECT_EQ(tx_meta1->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx_meta1->tx_hash(), tx_hash1_);

  // Send another tx.
  SetInterceptor(latest_blockhash1_, tx_hash2_);
  ApproveTransaction(meta_id2);
  base::RunLoop().RunUntilIdle();

  tx_meta2 = solana_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_EQ(*tx_meta2->tx(), *tx);
  EXPECT_EQ(tx_meta2->signature_status(), SolanaSignatureStatus());
  EXPECT_EQ(tx_meta2->from(), from_account);
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Submitted);
  EXPECT_EQ(tx_meta2->tx_hash(), tx_hash2_);

  // Fast forward to have block tracker run with current interceptor.
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));

  SetInterceptor(latest_blockhash2_, tx_hash2_, true);

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

}  // namespace brave_wallet
