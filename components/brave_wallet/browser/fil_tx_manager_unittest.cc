/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_manager.h"
#include <unordered_map>

#include <utility>

#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {
void EqualJSONs(const std::string& current_string,
                const std::string& expected_string) {
  auto current_json = base::JSONReader::Read(current_string);
  ASSERT_TRUE(current_json);
  auto expected_string_json = base::JSONReader::Read(expected_string);
  ASSERT_TRUE(expected_string_json);
  EXPECT_EQ(*current_json, *expected_string_json);
}
}  // namespace

class FilTxManagerUnitTest : public testing::Test {
 public:
  FilTxManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletFilecoinFeature);

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_.reset(
        new JsonRpcService(shared_url_loader_factory_, &prefs_));
    keyring_service_.reset(
        new KeyringService(json_rpc_service_.get(), &prefs_));
    tx_service_.reset(new TxService(json_rpc_service_.get(),
                                    keyring_service_.get(), &prefs_));

    base::RunLoop run_loop;
    json_rpc_service_->SetNetwork(brave_wallet::mojom::kLocalhostChainId,
                                  mojom::CoinType::FIL,
                                  base::BindLambdaForTesting([&](bool success) {
                                    EXPECT_TRUE(success);
                                    run_loop.Quit();
                                  }));
    run_loop.Run();
    keyring_service_->CreateWallet("testing123", base::DoNothing());
    base::RunLoop().RunUntilIdle();
    keyring_service_->AddAccount("Account 1", mojom::CoinType::FIL,
                                 base::DoNothing());
    base::RunLoop().RunUntilIdle();
  }

  std::string from() {
    return keyring_service_
        ->GetHDKeyringById(brave_wallet::mojom::kFilecoinKeyringId)
        ->GetAddress(0);
  }

  void SetInterceptor(const GURL& expected_url,
                      const std::string& expected_method,
                      const std::string& content) {
    ClearInterceptorResponses();
    AddInterceptorResponse(expected_method, content);
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, expected_url, expected_method,
         content](const network::ResourceRequest& request) {
          EXPECT_EQ(request.url, expected_url);
          std::string header_value;
          EXPECT_TRUE(request.headers.GetHeader("X-Eth-Method", &header_value));
          EXPECT_TRUE(responses_.count(header_value));
          url_loader_factory_.ClearResponses();
          auto response = responses_[header_value];
          url_loader_factory_.AddResponse(request.url.spec(), response);
        }));
  }

  void AddInterceptorResponse(const std::string& expected_method,
                              const std::string& content) {
    responses_[expected_method] = content;
  }

  void ClearInterceptorResponses() {
    responses_.clear();
    url_loader_factory_.ClearResponses();
  }

  void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      absl::optional<std::string> expected_message) {
    base::RunLoop run_loop;
    fil_tx_manager()->GetTransactionMessageToSign(
        tx_meta_id, base::BindLambdaForTesting(
                        [&](const absl::optional<std::string>& message) {
                          EXPECT_EQ(message.has_value(),
                                    expected_message.has_value());
                          if (expected_message.has_value()) {
                            EqualJSONs(*message, *expected_message);
                          }
                          run_loop.Quit();
                        }));
    run_loop.Run();
  }

  FilTxManager* fil_tx_manager() { return tx_service_->GetFilTxManager(); }

  PrefService* prefs() { return &prefs_; }

  void AddUnapprovedTransaction(mojom::FilTxDataPtr tx_data,
                                const std::string& from,
                                const absl::optional<url::Origin>& origin,
                                std::string* meta_id) {
    auto tx_data_union = mojom::TxDataUnion::NewFilTxData(std::move(tx_data));

    base::RunLoop run_loop;
    fil_tx_manager()->AddUnapprovedTransaction(
        std::move(tx_data_union), from, origin,
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
    fil_tx_manager()->ApproveTransaction(
        meta_id, base::BindLambdaForTesting(
                     [&](bool success, mojom::ProviderErrorUnionPtr error_union,
                         const std::string& err_message) {
                       ASSERT_TRUE(success);
                       ASSERT_TRUE(error_union->is_filecoin_provider_error());
                       ASSERT_EQ(error_union->get_filecoin_provider_error(),
                                 mojom::FilecoinProviderError::kSuccess);
                       ASSERT_TRUE(err_message.empty());
                       run_loop.Quit();
                     }));
    run_loop.Run();
  }
  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return brave_wallet::GetNetworkURL(prefs(), chain_id, coin);
  }
  void SetGasEstimateInterceptor(const std::string& from_account,
                                 const std::string& to_account) {
    std::string gas_response = R"({
                          "jsonrpc": "2.0",
                          "result": {
                            "Version": 0,
                            "To": "{to}",
                            "From": "{from}",
                            "Nonce": 5,
                            "Value": "42",
                            "GasLimit": 598585,
                            "GasFeeCap": "100820",
                            "GasPremium": "99766",
                            "Method": 0,
                            "Params": "",
                            "CID": {
                              "/": "bafy2bzacedkdoldmztwjwi3jvxhxo4qqp7haufuifpqzregfqkthlyhhf2lfu"
                            }
                          },
                          "id": 1
                        })";
    base::ReplaceSubstringsAfterOffset(&gas_response, 0, "{to}", to_account);
    base::ReplaceSubstringsAfterOffset(&gas_response, 0, "{from}",
                                       from_account);
    SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                   "Filecoin.GasEstimateMessageGas", gas_response);
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TxService> tx_service_;
  std::unordered_map<std::string, std::string> responses_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(FilTxManagerUnitTest, SubmitTransactions) {
  std::string from_account = from();
  std::string to_account = "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  SetGasEstimateInterceptor(from_account, to_account);
  auto tx_data = mojom::FilTxData::New(
      "" /* nonce */, "" /* gas_premium */, "" /* gas_fee_cap */,
      "" /* gas_limit */, "" /* max_fee */, to_account, from_account, "11");
  auto tx = FilTransaction::FromTxData(tx_data.Clone());

  std::string meta_id1;
  AddUnapprovedTransaction(tx_data.Clone(), from_account, absl::nullopt,
                           &meta_id1);

  auto tx_meta1 = fil_tx_manager()->GetTxForTesting(meta_id1);
  EXPECT_TRUE(tx_meta1);

  EXPECT_EQ(tx_meta1->tx()->gas_fee_cap(), "100820");
  EXPECT_EQ(tx_meta1->tx()->gas_limit(), 598585);
  EXPECT_EQ(tx_meta1->tx()->gas_premium(), "99766");
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Unapproved);

  std::string meta_id2;
  AddUnapprovedTransaction(tx_data.Clone(), from_account, absl::nullopt,
                           &meta_id2);
  auto tx_meta2 = fil_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_EQ(tx_meta2->from(), from_account);
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Unapproved);

  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.MpoolGetNonce",
                 R"({ "jsonrpc": "2.0", "id": 1, "result": 1 })");
  AddInterceptorResponse("Filecoin.MpoolPush",
                         R"({
            "id": 1,
            "jsonrpc": "2.0",
            "result":
            {
                "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
            }
        })");
  AddInterceptorResponse("Filecoin.StateSearchMsgLimited",
                         R"({
            "Message":
            {
                "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
            },
            "Receipt":
            {
                "ExitCode": 0
            }
        })");
  AddInterceptorResponse("Filecoin..ChainHead",
                         R"({
      "Blocks":[],
      "Cids": [{
            "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
      }],
      "Height": 22452
    })");

  ApproveTransaction(meta_id1);
  // Wait for tx to be updated.
  base::RunLoop().RunUntilIdle();
  tx_meta1 = fil_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_FALSE(tx_meta1->tx_hash().empty());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Submitted);

  // Send another tx.
  ApproveTransaction(meta_id2);
  base::RunLoop().RunUntilIdle();

  tx_meta2 = fil_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_EQ(tx_meta2->from(), from_account);
  EXPECT_FALSE(tx_meta2->tx_hash().empty());
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Submitted);
}

TEST_F(FilTxManagerUnitTest, WalletOrigin) {
  const std::string from_account = "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
  SetGasEstimateInterceptor(from_account, to_account);
  auto tx_data = mojom::FilTxData::New(
      "" /* nonce */, "" /* gas_premium */, "" /* gas_fee_cap */,
      "" /* gas_limit */, "" /* max_fee */, to_account, from_account, "11");
  std::string meta_id;
  AddUnapprovedTransaction(std::move(tx_data), from_account, absl::nullopt,
                           &meta_id);

  auto tx_meta = fil_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta);

  EXPECT_EQ(tx_meta->origin(), url::Origin::Create(GURL("chrome://wallet")));
}

TEST_F(FilTxManagerUnitTest, SomeSiteOrigin) {
  const std::string from_account = "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
  SetGasEstimateInterceptor(from_account, to_account);
  auto tx_data = mojom::FilTxData::New(
      "" /* nonce */, "" /* gas_premium */, "" /* gas_fee_cap */,
      "" /* gas_limit */, "" /* max_fee */, to_account, from_account, "11");
  std::string meta_id;
  AddUnapprovedTransaction(std::move(tx_data), from_account,
                           url::Origin::Create(GURL("https://some.site.com")),
                           &meta_id);

  auto tx_meta = fil_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->origin(),
            url::Origin::Create(GURL("https://some.site.com")));
}

TEST_F(FilTxManagerUnitTest, GetTransactionMessageToSign) {
  const std::string from_account = "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";

  auto tx_data = mojom::FilTxData::New(
      "1" /* nonce */, "2" /* gas_premium */, "3" /* gas_fee_cap */,
      "4" /* gas_limit */, "" /* max_fee */, to_account, from_account, "11");
  std::string meta_id;
  AddUnapprovedTransaction(std::move(tx_data), from_account, absl::nullopt,
                           &meta_id);
  auto tx_meta = fil_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->from(), from_account);
  EXPECT_EQ(tx_meta->status(), mojom::TransactionStatus::Unapproved);
  GetTransactionMessageToSign(meta_id, R"(
    {
        "From": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
        "GasFeeCap": "3",
        "GasLimit": 4,
        "GasPremium": "2",
        "MethodNum": 0,
        "Nonce": 1,
        "Params": "",
        "To": "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy",
        "Value": "11",
        "Version": 0
    }
  )");
  GetTransactionMessageToSign("unknown id", absl::nullopt);
  GetTransactionMessageToSign("", absl::nullopt);
}
}  //  namespace brave_wallet
