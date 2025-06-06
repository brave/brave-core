/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_rpc.h"

#include <memory>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/to_string.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "net/http/http_status_code.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::RunOnceClosure;
using base::test::TestFuture;
using testing::Truly;

namespace brave_wallet {
namespace {

std::string LatestBlockPayload(int height, int slot, int epoch) {
  base::Value::Dict result;
  result.Set("height", height);
  result.Set("slot", slot);
  result.Set("epoch", epoch);
  return base::ToString(result);
}

std::string LatestEpochParameters(int min_fee_a, int min_fee_b) {
  base::Value::Dict result;
  result.Set("min_fee_a", min_fee_a);
  result.Set("min_fee_b", min_fee_b);
  return base::ToString(result);
}

}  // namespace

class CardanoRpcUnitTest : public testing::Test {
 public:
  CardanoRpcUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~CardanoRpcUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    mainnet_rpc_url_ =
        network_manager_
            ->GetNetworkURL(mojom::kCardanoMainnet, mojom::CoinType::ADA)
            .spec();
    testnet_rpc_url_ =
        network_manager_
            ->GetNetworkURL(mojom::kCardanoTestnet, mojom::CoinType::ADA)
            .spec();
    cardano_mainnet_rpc_ = std::make_unique<cardano_rpc::CardanoRpc>(
        mojom::kCardanoMainnet, *network_manager_, shared_url_loader_factory_);
    cardano_testnet_rpc_ = std::make_unique<cardano_rpc::CardanoRpc>(
        mojom::kCardanoTestnet, *network_manager_, shared_url_loader_factory_);
  }

  std::string GetResponseString() const {
    return base::NumberToString(response_height_);
  }

  network::ResourceRequest WaitForPendingRequest() {
    EXPECT_TRUE(
        base::test::RunUntil([&] { return url_loader_factory_.NumPending(); }));
    return url_loader_factory_.GetPendingRequest(0)->request;
  }

 protected:
  std::string mainnet_rpc_url_;
  std::string testnet_rpc_url_;
  uint32_t response_height_ = 0;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<cardano_rpc::CardanoRpc> cardano_mainnet_rpc_;
  std::unique_ptr<cardano_rpc::CardanoRpc> cardano_testnet_rpc_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(CardanoRpcUnitTest, Throttling) {
  // For mainnet there is no throttling and always 5 requests.
  struct {
    const bool mainnet;
    const char* param;
    const size_t expected_size;
  } test_cases[] = {{true, "0", 5},  {true, "3", 5},  {true, "10", 5},
                    {false, "0", 5}, {false, "3", 3}, {false, "10", 5}};

  testnet_rpc_url_ = "https://cardano-test.example.com/api/";
  network_manager_->SetNetworkURLForTesting(mojom::kCardanoTestnet,
                                            GURL(testnet_rpc_url_));

  for (const auto& test_case : test_cases) {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeaturesAndParameters(
        {{features::kBraveWalletCardanoFeature,
          {{features::kCardanoRpcThrottle.name, test_case.param}}}},
        {});

    base::MockCallback<cardano_rpc::CardanoRpc::GetLatestBlockCallback>
        callback;

    const std::string req_url =
        (test_case.mainnet ? mainnet_rpc_url_ : testnet_rpc_url_) +
        "blocks/latest";

    url_loader_factory_.ClearResponses();

    auto* cardano_rpc = (test_case.mainnet ? cardano_mainnet_rpc_.get()
                                           : cardano_testnet_rpc_.get());

    // GetLatestBlock works.
    EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                  return arg.value() == cardano_rpc::Block{.height = 123,
                                                           .slot = 7,
                                                           .epoch = 88};
                })))
        .Times(5)
        .WillOnce({})
        .WillOnce({})
        .WillOnce({})
        .WillOnce({})
        .WillOnce(RunOnceClosure(task_environment_.QuitClosure()));
    cardano_rpc->GetLatestBlock(callback.Get());
    cardano_rpc->GetLatestBlock(callback.Get());
    cardano_rpc->GetLatestBlock(callback.Get());
    cardano_rpc->GetLatestBlock(callback.Get());
    cardano_rpc->GetLatestBlock(callback.Get());

    EXPECT_EQ(url_loader_factory_.pending_requests()->size(),
              test_case.expected_size);
    url_loader_factory_.AddResponse(req_url, LatestBlockPayload(123, 7, 88));
    task_environment_.RunUntilQuit();
    testing::Mock::VerifyAndClearExpectations(&callback);
  }
}

TEST_F(CardanoRpcUnitTest, BraveServicesKey) {
  base::MockCallback<cardano_rpc::CardanoRpc::GetLatestBlockCallback> callback;

  network_manager_->SetNetworkURLForTesting(
      mojom::kCardanoMainnet, GURL("https://cardano-test.example.com/api/"));
  cardano_mainnet_rpc_->GetLatestBlock(callback.Get());
  EXPECT_EQ(url_loader_factory_.pending_requests()->size(), 1u);
  EXPECT_FALSE(
      url_loader_factory_.pending_requests()->front().request.headers.GetHeader(
          kBraveServicesKeyHeader));

  url_loader_factory_.pending_requests()->clear();
  network_manager_->SetNetworkURLForTesting(mojom::kCardanoMainnet, GURL());

  cardano_mainnet_rpc_->GetLatestBlock(callback.Get());
  EXPECT_EQ(url_loader_factory_.pending_requests()->size(), 1u);
  EXPECT_TRUE(
      url_loader_factory_.pending_requests()->front().request.headers.GetHeader(
          kBraveServicesKeyHeader));
}

TEST_F(CardanoRpcUnitTest, GetLatestBlock) {
  const std::string req_url = mainnet_rpc_url_ + "blocks/latest";

  TestFuture<base::expected<cardano_rpc::Block, std::string>> block_future;

  // GetLatestBlock works.
  url_loader_factory_.AddResponse(req_url, LatestBlockPayload(123, 7, 88));
  cardano_mainnet_rpc_->GetLatestBlock(block_future.GetCallback());
  EXPECT_EQ(block_future.Take().value(),
            (cardano_rpc::Block{.height = 123, .slot = 7, .epoch = 88}));

  // GetLatestBlock works.
  url_loader_factory_.AddResponse(req_url, LatestBlockPayload(9999999, 5, 12));
  cardano_mainnet_rpc_->GetLatestBlock(block_future.GetCallback());
  EXPECT_EQ(block_future.Take().value(),
            (cardano_rpc::Block{.height = 9999999, .slot = 5, .epoch = 12}));

  // Invalid value returned.
  url_loader_factory_.AddResponse(req_url, R"({"some": "string"})");
  cardano_mainnet_rpc_->GetLatestBlock(block_future.GetCallback());
  EXPECT_EQ(block_future.Take().error(), WalletParsingErrorMessage());

  // HTTP Error returned.
  url_loader_factory_.AddResponse(req_url, LatestBlockPayload(123, 7, 88),
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  cardano_mainnet_rpc_->GetLatestBlock(block_future.GetCallback());
  EXPECT_EQ(block_future.Take().error(), WalletInternalErrorMessage());

  // Testnet works.
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "blocks/latest",
                                  LatestBlockPayload(123, 7, 88));
  cardano_testnet_rpc_->GetLatestBlock(block_future.GetCallback());
  EXPECT_EQ(block_future.Take().value(),
            (cardano_rpc::Block{.height = 123, .slot = 7, .epoch = 88}));
}

TEST_F(CardanoRpcUnitTest, GetLatestEpochParameters) {
  const std::string req_url = mainnet_rpc_url_ + "epochs/latest/parameters";

  TestFuture<base::expected<cardano_rpc::EpochParameters, std::string>>
      epoch_params_future;

  // GetLatestEpochParameters works.
  url_loader_factory_.AddResponse(req_url, LatestEpochParameters(100, 200));
  cardano_mainnet_rpc_->GetLatestEpochParameters(
      epoch_params_future.GetCallback());
  EXPECT_EQ(epoch_params_future.Take().value(),
            (cardano_rpc::EpochParameters{.min_fee_coefficient = 100,
                                          .min_fee_constant = 200}));

  // GetLatestEpochParameters works.
  url_loader_factory_.AddResponse(req_url, LatestEpochParameters(7, 5));
  cardano_mainnet_rpc_->GetLatestEpochParameters(
      epoch_params_future.GetCallback());
  EXPECT_EQ(epoch_params_future.Take().value(),
            (cardano_rpc::EpochParameters{.min_fee_coefficient = 7,
                                          .min_fee_constant = 5}));

  // Invalid value returned.
  url_loader_factory_.AddResponse(req_url, R"({"some": "string"})");
  cardano_mainnet_rpc_->GetLatestEpochParameters(
      epoch_params_future.GetCallback());
  EXPECT_EQ(epoch_params_future.Take().error(), WalletParsingErrorMessage());

  // HTTP Error returned.
  url_loader_factory_.AddResponse(req_url, LatestEpochParameters(123, 7),
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  cardano_mainnet_rpc_->GetLatestEpochParameters(
      epoch_params_future.GetCallback());
  EXPECT_EQ(epoch_params_future.Take().error(), WalletInternalErrorMessage());

  // Testnet works.
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "epochs/latest/parameters",
                                  LatestEpochParameters(100, 200));
  cardano_testnet_rpc_->GetLatestEpochParameters(
      epoch_params_future.GetCallback());
  EXPECT_EQ(epoch_params_future.Take().value(),
            (cardano_rpc::EpochParameters{.min_fee_coefficient = 100,
                                          .min_fee_constant = 200}));
}

TEST_F(CardanoRpcUnitTest, GetUtxoList) {
  const std::string address =
      "addr_"
      "test1qqy6nhfyks7wdu3dudslys37v252w2nwhv0fw2nfawemmn8k8ttq8f3gag0h89aepvx"
      "3xf69g0l9pf80tqv7cve0l33sw96paj";
  const std::string req_url =
      mainnet_rpc_url_ + "addresses/" + address + "/utxos";

  const std::string utxo_json = R"([
    {
      "tx_hash": "1fca84164f59606710ff4cf0fd660753bd299e30bb2c8194117fdb965ace67b9",
      "output_index": 2,
      "amount": [{
        "quantity": 406560,
        "unit": "lovelace"
      }]
    },{
      "tx_hash": "f80875bfaa0726fadc0068cca851f3252762670df345e6c7a483fe841af98e98",
      "output_index": 1,
      "amount": [{
        "quantity": 2407560,
        "unit": "lovelace"
      }]
    }
  ])";

  cardano_rpc::UnspentOutputs utxos;
  utxos.emplace_back();
  utxos.back().tx_hash = test::HexToArray<32>(
      "1fca84164f59606710ff4cf0fd660753bd299e30bb2c8194117fdb965ace67b9");
  utxos.back().output_index = 2;
  utxos.back().lovelace_amount = 406560;

  utxos.emplace_back();
  utxos.back().tx_hash = test::HexToArray<32>(
      "f80875bfaa0726fadc0068cca851f3252762670df345e6c7a483fe841af98e98");
  utxos.back().output_index = 1;
  utxos.back().lovelace_amount = 2407560;

  TestFuture<base::expected<cardano_rpc::UnspentOutputs, std::string>>
      utxos_future;

  // GetUtxoList works.
  url_loader_factory_.AddResponse(req_url, utxo_json);
  cardano_mainnet_rpc_->GetUtxoList(address, utxos_future.GetCallback());
  EXPECT_EQ(utxos_future.Take().value(), utxos);

  // Invalid value returned.
  url_loader_factory_.AddResponse(req_url, "[123]");
  cardano_mainnet_rpc_->GetUtxoList(address, utxos_future.GetCallback());
  EXPECT_EQ(utxos_future.Take().error(), WalletParsingErrorMessage());

  // HTTP Error returned.
  url_loader_factory_.AddResponse(req_url, utxo_json,
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  cardano_mainnet_rpc_->GetUtxoList(address, utxos_future.GetCallback());
  EXPECT_EQ(utxos_future.Take().error(), WalletInternalErrorMessage());

  // HTTP 404 Not Found Error results in empty list.
  url_loader_factory_.AddResponse(req_url, "Not Found", net::HTTP_NOT_FOUND);
  cardano_mainnet_rpc_->GetUtxoList(address, utxos_future.GetCallback());
  EXPECT_TRUE(utxos_future.Take().value().empty());

  // Testnet works.
  url_loader_factory_.ClearResponses();
  url_loader_factory_.AddResponse(
      testnet_rpc_url_ + "addresses/" + address + "/utxos", utxo_json);
  cardano_testnet_rpc_->GetUtxoList(address, utxos_future.GetCallback());
  EXPECT_EQ(utxos_future.Take().value(), utxos);
}

TEST_F(CardanoRpcUnitTest, PostTransaction) {
  const std::string req_url = mainnet_rpc_url_ + "tx/submit";
  const std::string txid =
      "1fca84164f59606710ff4cf0fd660753bd299e30bb2c8194117fdb965ace67b9";
  const std::string txid_response = "\"" + txid + "\"";

  TestFuture<base::expected<std::string, std::string>> post_tx_future;

  // PostTransaction works.
  cardano_mainnet_rpc_->PostTransaction({1, 2, 3},
                                        post_tx_future.GetCallback());
  auto request = WaitForPendingRequest();
  EXPECT_EQ(request.url, req_url);
  EXPECT_EQ(request.request_body->elements()
                ->at(0)
                .As<network::DataElementBytes>()
                .AsStringPiece(),
            "\x1\x2\x3");
  url_loader_factory_.AddResponse(req_url, txid_response);
  EXPECT_EQ(post_tx_future.Take().value(), txid);

  // Invalid value returned.
  url_loader_factory_.ClearResponses();
  cardano_mainnet_rpc_->PostTransaction({1, 2, 3},
                                        post_tx_future.GetCallback());
  request = WaitForPendingRequest();
  EXPECT_EQ(request.url, req_url);
  url_loader_factory_.AddResponse(req_url, "not valid txid");
  EXPECT_EQ(post_tx_future.Take().error(), WalletParsingErrorMessage());

  // HTTP Error returned.
  url_loader_factory_.ClearResponses();
  cardano_mainnet_rpc_->PostTransaction({1, 2, 3},
                                        post_tx_future.GetCallback());
  request = WaitForPendingRequest();
  EXPECT_EQ(request.url, req_url);
  url_loader_factory_.AddResponse(req_url, txid_response,
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  EXPECT_EQ(post_tx_future.Take().error(), WalletInternalErrorMessage());

  // Testnet works.
  url_loader_factory_.ClearResponses();
  cardano_testnet_rpc_->PostTransaction({1, 2, 3},
                                        post_tx_future.GetCallback());
  request = WaitForPendingRequest();
  EXPECT_EQ(request.url, testnet_rpc_url_ + "tx/submit");
  EXPECT_EQ(request.request_body->elements()
                ->at(0)
                .As<network::DataElementBytes>()
                .AsStringPiece(),
            "\x1\x2\x3");
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "tx/submit",
                                  txid_response);
  EXPECT_EQ(post_tx_future.Take().value(), txid);
}

}  // namespace brave_wallet
