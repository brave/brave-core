/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"

#include <memory>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Truly;

namespace brave_wallet {
namespace {
auto MatchError(const std::string& error) {
  return Truly([=](auto& arg) { return arg.error() == error; });
}

}  // namespace

class BitcoinRpcUnitTest : public testing::Test {
 public:
  BitcoinRpcUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~BitcoinRpcUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    bitcoin_rpc_ = std::make_unique<bitcoin_rpc::BitcoinRpc>(
        &prefs_, shared_url_loader_factory_);

    auto btc_mainnet =
        GetKnownChain(&prefs_, mojom::kBitcoinMainnet, mojom::CoinType::BTC);
    btc_mainnet->rpc_endpoints[0] = GURL(mainnet_rpc_url_);
    AddCustomNetwork(&prefs_, *btc_mainnet);
    auto btc_testnet =
        GetKnownChain(&prefs_, mojom::kBitcoinTestnet, mojom::CoinType::BTC);
    btc_testnet->rpc_endpoints[0] = GURL(testnet_rpc_url_);
    AddCustomNetwork(&prefs_, *btc_testnet);
  }

  std::string GetResponseString() const {
    return base::NumberToString(response_height_);
  }

 protected:
  std::string mainnet_rpc_url_ = "https://btc-mainnet.com/api/";
  std::string testnet_rpc_url_ = "https://btc-testnet.com/api/";
  uint32_t response_height_ = 0;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<bitcoin_rpc::BitcoinRpc> bitcoin_rpc_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(BitcoinRpcUnitTest, Throttling) {
  using GetChainHeightResult = base::expected<uint32_t, std::string>;

  struct {
    const char* param;
    const size_t expected_size;
  } test_cases[] = {{"0", 5}, {"3", 3}, {"10", 5}};

  for (auto& test_case : test_cases) {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeaturesAndParameters(
        {{features::kBraveWalletBitcoinFeature,
          {{features::kBitcoinRpcThrottle.name, test_case.param}}}},
        {});

    base::MockCallback<bitcoin_rpc::BitcoinRpc::GetChainHeightCallback>
        callback;

    const std::string req_url = mainnet_rpc_url_ + "blocks/tip/height";

    url_loader_factory_.ClearResponses();

    // GetChainHeight works.
    EXPECT_CALL(callback, Run(GetChainHeightResult(base::ok(123)))).Times(5);
    bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, callback.Get());
    bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, callback.Get());
    bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, callback.Get());
    bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, callback.Get());
    bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, callback.Get());
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(url_loader_factory_.pending_requests()->size(),
              test_case.expected_size);
    url_loader_factory_.AddResponse(req_url, "123");
    base::RunLoop().RunUntilIdle();
    testing::Mock::VerifyAndClearExpectations(&callback);
  }
}

TEST_F(BitcoinRpcUnitTest, GetChainHeight) {
  using GetChainHeightResult = base::expected<uint32_t, std::string>;
  base::MockCallback<bitcoin_rpc::BitcoinRpc::GetChainHeightCallback> callback;

  const std::string req_url = mainnet_rpc_url_ + "blocks/tip/height";

  // GetChainHeight works.
  EXPECT_CALL(callback, Run(GetChainHeightResult(base::ok(123))));
  url_loader_factory_.AddResponse(req_url, "123");
  bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // GetChainHeight works.
  EXPECT_CALL(callback, Run(GetChainHeightResult(base::ok(9999999))));
  url_loader_factory_.AddResponse(req_url, "9999999");
  bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid value returned.
  EXPECT_CALL(callback,
              Run(GetChainHeightResult(base::unexpected("Invalid json"))));
  url_loader_factory_.AddResponse(req_url, "some string");
  bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // HTTP Error returned.
  EXPECT_CALL(callback, Run(GetChainHeightResult(base::unexpected(
                            "Unexpected HTTP result code 500"))));
  url_loader_factory_.AddResponse(req_url, "123",
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Testent works.
  EXPECT_CALL(callback, Run(GetChainHeightResult(base::ok(123))));
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "blocks/tip/height",
                                  "123");
  bitcoin_rpc_->GetChainHeight(mojom::kBitcoinTestnet, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid chain fails.
  EXPECT_CALL(callback,
              Run(GetChainHeightResult(base::unexpected("Internal Error"))));
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetChainHeight("0x123", callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinRpcUnitTest, GetTransaction) {
  base::MockCallback<bitcoin_rpc::BitcoinRpc::GetTransactionCallback> callback;

  const std::string txid =
      "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5";
  const std::string req_url = mainnet_rpc_url_ + "tx/" + txid;

  const std::string tx_json = R"({
    "txid": "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5",
    "dummy": 123,
    "status" : {
      "confirmed" : true
    }
  }
  )";

  // GetTransaction works.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                bitcoin_rpc::Transaction tx;
                tx.txid = txid;
                tx.status.confirmed = true;
                return arg.value().ToValue() == tx.ToValue();
              })));
  url_loader_factory_.AddResponse(req_url, tx_json);
  bitcoin_rpc_->GetTransaction(mojom::kBitcoinMainnet, txid, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid value returned.
  EXPECT_CALL(callback, Run(MatchError("Invalid json")));
  url_loader_factory_.AddResponse(req_url, "some string");
  bitcoin_rpc_->GetTransaction(mojom::kBitcoinMainnet, txid, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // HTTP Error returned.
  EXPECT_CALL(callback, Run(MatchError("Unexpected HTTP result code 500")));
  url_loader_factory_.AddResponse(req_url, tx_json,
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  bitcoin_rpc_->GetTransaction(mojom::kBitcoinMainnet, txid, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Testent works.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                bitcoin_rpc::Transaction tx;
                tx.txid = txid;
                tx.status.confirmed = true;
                return arg.value().ToValue() == tx.ToValue();
              })));
  url_loader_factory_.ClearResponses();
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "tx/" + txid, tx_json);
  bitcoin_rpc_->GetTransaction(mojom::kBitcoinTestnet, txid, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid chain fails.
  EXPECT_CALL(callback, Run(MatchError("Internal Error")));
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetTransaction("0x123", txid, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid txid arg format fails.
  EXPECT_CALL(callback, Run(MatchError("Internal Error")));
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetTransaction(mojom::kBitcoinMainnet, txid + "/",
                               callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinRpcUnitTest, GetAddressStats) {
  base::MockCallback<bitcoin_rpc::BitcoinRpc::GetAddressStatsCallback> callback;

  const std::string address = "tb1q56kslnp386v43wpp6wkpx072ryud5gu865efx8";
  const std::string req_url = mainnet_rpc_url_ + "address/" + address;

  const std::string address_json = R"({
    "address": "tb1q56kslnp386v43wpp6wkpx072ryud5gu865efx8",
    "chain_stats": {
      "funded_txo_count": 1,
      "funded_txo_sum": 2,
      "spent_txo_count": 3,
      "spent_txo_sum": 4,
      "tx_count": 5
    },
    "mempool_stats": {
      "funded_txo_count": 6,
      "funded_txo_sum": 7,
      "spent_txo_count": 8,
      "spent_txo_sum": 9,
      "tx_count": 10
    }
  })";

  bitcoin_rpc::AddressStats stats;
  stats.address = address;
  stats.chain_stats.funded_txo_sum = "2";
  stats.chain_stats.spent_txo_sum = "4";
  stats.chain_stats.tx_count = "5";
  stats.mempool_stats.funded_txo_sum = "7";
  stats.mempool_stats.spent_txo_sum = "9";
  stats.mempool_stats.tx_count = "10";

  // GetAddressStats works.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                return arg.value().ToValue() == stats.ToValue();
              })));
  url_loader_factory_.AddResponse(req_url, address_json);
  bitcoin_rpc_->GetAddressStats(mojom::kBitcoinMainnet, address,
                                callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid value returned.
  EXPECT_CALL(callback, Run(MatchError("Invalid json")));
  url_loader_factory_.AddResponse(req_url, "[123]");
  bitcoin_rpc_->GetAddressStats(mojom::kBitcoinMainnet, address,
                                callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // HTTP Error returned.
  EXPECT_CALL(callback, Run(MatchError("Unexpected HTTP result code 500")));
  url_loader_factory_.AddResponse(req_url, address_json,
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  bitcoin_rpc_->GetAddressStats(mojom::kBitcoinMainnet, address,
                                callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Testent works.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                return arg.value().ToValue() == stats.ToValue();
              })));
  url_loader_factory_.ClearResponses();
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "address/" + address,
                                  address_json);
  bitcoin_rpc_->GetAddressStats(mojom::kBitcoinTestnet, address,
                                callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid chain fails.
  EXPECT_CALL(callback, Run(MatchError("Internal Error")));
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetAddressStats("0x123", address, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid address arg format fails.
  EXPECT_CALL(callback, Run(MatchError("Internal Error")));
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetAddressStats(mojom::kBitcoinMainnet, address + "/",
                                callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinRpcUnitTest, GetUtxoList) {
  base::MockCallback<bitcoin_rpc::BitcoinRpc::GetUtxoListCallback> callback;

  const std::string address = "tb1q56kslnp386v43wpp6wkpx072ryud5gu865efx8";
  const std::string req_url = mainnet_rpc_url_ + "address/" + address + "/utxo";

  const std::string utxo_json = R"([
    {
      "txid": "1fca84164f59606710ff4cf0fd660753bd299e30bb2c8194117fdb965ace67b9",
      "vout": 2,
      "status": {
        "confirmed": false,
        "block_height": 2474738,
        "block_hash":
            "000000000000000b76eff8d4e99f35b7d918e56497057dc9a042bb6fb0b67733",
        "block_time": 1692877128
      },
      "value": 406560
    },{
      "txid": "f80875bfaa0726fadc0068cca851f3252762670df345e6c7a483fe841af98e98",
      "vout": 1,
      "status": {
        "confirmed": true,
        "block_height": 2474734,
        "block_hash":
            "000000000000000e4827189881909630974e4cc93953642f715fd86464a52808",
        "block_time": 1692873891
      },
      "value": 2407560
    }
  ])";

  std::vector<bitcoin_rpc::UnspentOutput> utxos;
  utxos.emplace_back();
  utxos.back().txid =
      "1fca84164f59606710ff4cf0fd660753bd299e30bb2c8194117fdb965ace67b9";
  utxos.back().vout = "2";
  utxos.back().value = "406560";
  utxos.back().status.confirmed = false;
  utxos.emplace_back();
  utxos.back().txid =
      "f80875bfaa0726fadc0068cca851f3252762670df345e6c7a483fe841af98e98";
  utxos.back().vout = "1";
  utxos.back().value = "2407560";
  utxos.back().status.confirmed = true;

  // GetUtxoList works.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                return arg.value().size() == 2 &&
                       arg.value()[0].ToValue() == utxos[0].ToValue() &&
                       arg.value()[1].ToValue() == utxos[1].ToValue();
              })));
  url_loader_factory_.AddResponse(req_url, utxo_json);
  bitcoin_rpc_->GetUtxoList(mojom::kBitcoinMainnet, address, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid value returned.
  EXPECT_CALL(callback, Run(MatchError("Invalid json")));
  url_loader_factory_.AddResponse(req_url, "[123]");
  bitcoin_rpc_->GetUtxoList(mojom::kBitcoinMainnet, address, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // HTTP Error returned.
  EXPECT_CALL(callback, Run(MatchError("Unexpected HTTP result code 500")));
  url_loader_factory_.AddResponse(req_url, utxo_json,
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  bitcoin_rpc_->GetUtxoList(mojom::kBitcoinMainnet, address, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Testent works.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                return arg.value().size() == 2 &&
                       arg.value()[0].ToValue() == utxos[0].ToValue() &&
                       arg.value()[1].ToValue() == utxos[1].ToValue();
              })));
  url_loader_factory_.ClearResponses();
  url_loader_factory_.AddResponse(
      testnet_rpc_url_ + "address/" + address + "/utxo", utxo_json);
  bitcoin_rpc_->GetUtxoList(mojom::kBitcoinTestnet, address, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid chain fails.
  EXPECT_CALL(callback, Run(MatchError("Internal Error")));
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetUtxoList("0x123", address, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid address arg format fails.
  EXPECT_CALL(callback, Run(MatchError("Internal Error")));
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetUtxoList(mojom::kBitcoinMainnet, address + "/",
                            callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinRpcUnitTest, PostTransaction) {
  base::MockCallback<bitcoin_rpc::BitcoinRpc::PostTransactionCallback> callback;

  const std::string req_url = mainnet_rpc_url_ + "tx";
  const std::string txid =
      "1fca84164f59606710ff4cf0fd660753bd299e30bb2c8194117fdb965ace67b9";

  // PostTransaction works.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) { return arg == txid; })));
  bitcoin_rpc_->PostTransaction(mojom::kBitcoinMainnet, {1, 2, 3},
                                callback.Get());
  base::RunLoop().RunUntilIdle();
  auto request = url_loader_factory_.GetPendingRequest(0)->request;
  EXPECT_EQ(request.url, req_url);
  EXPECT_EQ(request.request_body->elements()
                ->at(0)
                .As<network::DataElementBytes>()
                .AsStringPiece(),
            "010203");
  url_loader_factory_.AddResponse(req_url, txid);
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid value returned.
  url_loader_factory_.ClearResponses();
  EXPECT_CALL(callback, Run(MatchError("Invalid json")));
  bitcoin_rpc_->PostTransaction(mojom::kBitcoinMainnet, {1, 2, 3},
                                callback.Get());
  base::RunLoop().RunUntilIdle();
  request = url_loader_factory_.GetPendingRequest(0)->request;
  EXPECT_EQ(request.url, req_url);
  url_loader_factory_.AddResponse(req_url, "not valid txid");
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // HTTP Error returned.
  url_loader_factory_.ClearResponses();
  EXPECT_CALL(callback, Run(MatchError("Unexpected HTTP result code 500")));
  bitcoin_rpc_->PostTransaction(mojom::kBitcoinMainnet, {1, 2, 3},
                                callback.Get());
  base::RunLoop().RunUntilIdle();
  request = url_loader_factory_.GetPendingRequest(0)->request;
  EXPECT_EQ(request.url, req_url);
  url_loader_factory_.AddResponse(req_url, txid,
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Testent works.
  url_loader_factory_.ClearResponses();
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) { return arg == txid; })));
  bitcoin_rpc_->PostTransaction(mojom::kBitcoinTestnet, {1, 2, 3},
                                callback.Get());
  base::RunLoop().RunUntilIdle();
  request = url_loader_factory_.GetPendingRequest(0)->request;
  EXPECT_EQ(request.url, testnet_rpc_url_ + "tx");
  EXPECT_EQ(request.request_body->elements()
                ->at(0)
                .As<network::DataElementBytes>()
                .AsStringPiece(),
            "010203");
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "tx", txid);
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid chain fails.
  url_loader_factory_.ClearResponses();
  EXPECT_CALL(callback, Run(MatchError("Internal Error")));
  bitcoin_rpc_->PostTransaction("0x123", {1, 2, 3}, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

}  // namespace brave_wallet
