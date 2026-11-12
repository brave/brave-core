/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

template <typename T>
void ExpectError(const base::expected<T, std::string>& result,
                 const std::string& error) {
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), error);
}

std::string InternalError() {
  return WalletInternalErrorMessage();
}

std::string ParsingError() {
  return WalletParsingErrorMessage();
}

}  // namespace

class BitcoinRpcUnitTest : public testing::Test {
 public:
  BitcoinRpcUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~BitcoinRpcUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    bitcoin_rpc_ = std::make_unique<bitcoin_rpc::BitcoinRpc>(
        *network_manager_, url_loader_factory_.GetSafeWeakWrapper());

    mainnet_rpc_url_ =
        network_manager_
            ->GetKnownChain(mojom::kBitcoinMainnet, mojom::CoinType::BTC)
            ->rpc_endpoints.front()
            .spec();
    testnet_rpc_url_ =
        network_manager_
            ->GetKnownChain(mojom::kBitcoinTestnet, mojom::CoinType::BTC)
            ->rpc_endpoints.front()
            .spec();
  }

  std::string GetResponseString() const {
    return base::NumberToString(response_height_);
  }

 protected:
  std::string mainnet_rpc_url_;
  std::string testnet_rpc_url_;
  uint32_t response_height_ = 0;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<bitcoin_rpc::BitcoinRpc> bitcoin_rpc_;
};

TEST_F(BitcoinRpcUnitTest, Throttling) {
  using GetChainHeightResult = base::expected<uint32_t, std::string>;

  // For mainnet there is no throttling and always 5 requests.
  struct {
    const bool mainnet;
    const char* param;
    const size_t expected_size;
  } test_cases[] = {{true, "0", 5},  {true, "3", 5},  {true, "10", 5},
                    {false, "0", 5}, {false, "3", 3}, {false, "10", 5}};

  for (auto& test_case : test_cases) {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeaturesAndParameters(
        {{features::kBraveWalletBitcoinFeature,
          {{features::kBitcoinRpcThrottle.name, test_case.param}}}},
        {});

    std::array<base::test::TestFuture<GetChainHeightResult>, 5> futures;

    const std::string req_url =
        (test_case.mainnet ? mainnet_rpc_url_ : testnet_rpc_url_) +
        "blocks/tip/height";

    url_loader_factory_.ClearResponses();

    auto* chain_id =
        (test_case.mainnet ? mojom::kBitcoinMainnet : mojom::kBitcoinTestnet);

    // GetChainHeight works.
    for (auto& future : futures) {
      bitcoin_rpc_->GetChainHeight(chain_id, future.GetCallback());
    }

    ASSERT_TRUE(base::test::RunUntil([&] {
      return url_loader_factory_.pending_requests()->size() ==
             test_case.expected_size;
    }));
    EXPECT_EQ(url_loader_factory_.pending_requests()->size(),
              test_case.expected_size);
    url_loader_factory_.AddResponse(req_url, "123");

    for (auto& future : futures) {
      EXPECT_EQ(future.Take(), GetChainHeightResult(base::ok(123)));
    }
  }
}

TEST_F(BitcoinRpcUnitTest, GetChainHeight) {
  using GetChainHeightResult = base::expected<uint32_t, std::string>;
  base::test::TestFuture<GetChainHeightResult> future;

  const std::string req_url = mainnet_rpc_url_ + "blocks/tip/height";

  // GetChainHeight works.
  url_loader_factory_.AddResponse(req_url, "123");
  bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, future.GetCallback());
  EXPECT_EQ(future.Take(), GetChainHeightResult(base::ok(123)));

  // GetChainHeight works.
  url_loader_factory_.AddResponse(req_url, "9999999");
  bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, future.GetCallback());
  EXPECT_EQ(future.Take(), GetChainHeightResult(base::ok(9999999)));

  // Invalid value returned.
  url_loader_factory_.AddResponse(req_url, "some string");
  bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, future.GetCallback());
  ExpectError(future.Take(), ParsingError());

  // HTTP Error returned.
  url_loader_factory_.AddResponse(req_url, "123",
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  bitcoin_rpc_->GetChainHeight(mojom::kBitcoinMainnet, future.GetCallback());
  ExpectError(future.Take(), InternalError());

  // Testnet works.
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "blocks/tip/height",
                                  "123");
  bitcoin_rpc_->GetChainHeight(mojom::kBitcoinTestnet, future.GetCallback());
  EXPECT_EQ(future.Take(), GetChainHeightResult(base::ok(123)));

  // Invalid chain fails.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetChainHeight("0x123", future.GetCallback());
  ExpectError(future.Take(), InternalError());
}

TEST_F(BitcoinRpcUnitTest, GetFeeEstimates) {
  using GetFeeEstimatesResult =
      base::expected<std::map<uint32_t, double>, std::string>;
  base::test::TestFuture<GetFeeEstimatesResult> future;

  const std::string req_url = mainnet_rpc_url_ + "fee-estimates";

  const std::string estimates_json = R"({
    "1": 123.45,
    "2": 5.5,
    "123": 1
  })";

  std::map<uint32_t, double> estimates;
  estimates[1] = 123.45;
  estimates[2] = 5.5;
  estimates[123] = 1;

  // GetFeeEstimates works.
  url_loader_factory_.AddResponse(req_url, estimates_json);
  bitcoin_rpc_->GetFeeEstimates(mojom::kBitcoinMainnet, future.GetCallback());
  EXPECT_EQ(future.Take(), GetFeeEstimatesResult(base::ok(estimates)));

  // Invalid value returned.
  url_loader_factory_.AddResponse(req_url, "some string");
  bitcoin_rpc_->GetFeeEstimates(mojom::kBitcoinMainnet, future.GetCallback());
  ExpectError(future.Take(), ParsingError());

  // Non-integer key fails.
  url_loader_factory_.AddResponse(req_url, R"({"a": 1})");
  bitcoin_rpc_->GetFeeEstimates(mojom::kBitcoinMainnet, future.GetCallback());
  ExpectError(future.Take(), ParsingError());

  // Non-double key fails.
  url_loader_factory_.AddResponse(req_url, R"({"1": "a"})");
  bitcoin_rpc_->GetFeeEstimates(mojom::kBitcoinMainnet, future.GetCallback());
  ExpectError(future.Take(), ParsingError());

  // Empty dict fails.
  url_loader_factory_.AddResponse(req_url, R"({})");
  bitcoin_rpc_->GetFeeEstimates(mojom::kBitcoinMainnet, future.GetCallback());
  ExpectError(future.Take(), ParsingError());

  // List fails.
  url_loader_factory_.AddResponse(req_url, R"([{"1": 1}])");
  bitcoin_rpc_->GetFeeEstimates(mojom::kBitcoinMainnet, future.GetCallback());
  ExpectError(future.Take(), ParsingError());

  // HTTP Error returned.
  url_loader_factory_.AddResponse(req_url, "123",
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  bitcoin_rpc_->GetFeeEstimates(mojom::kBitcoinMainnet, future.GetCallback());
  ExpectError(future.Take(), InternalError());

  // Testnet works.
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "fee-estimates",
                                  estimates_json);
  bitcoin_rpc_->GetFeeEstimates(mojom::kBitcoinTestnet, future.GetCallback());
  EXPECT_EQ(future.Take(), GetFeeEstimatesResult(base::ok(estimates)));

  // Invalid chain fails.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetFeeEstimates("0x123", future.GetCallback());
  ExpectError(future.Take(), InternalError());
}

TEST_F(BitcoinRpcUnitTest, GetTransaction) {
  using GetTransactionResult =
      base::expected<bitcoin_rpc::Transaction, std::string>;
  base::test::TestFuture<GetTransactionResult> future;

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

  bitcoin_rpc::Transaction expected_tx;
  expected_tx.txid = txid;
  expected_tx.status.confirmed = true;

  // GetTransaction works.
  url_loader_factory_.AddResponse(req_url, tx_json);
  bitcoin_rpc_->GetTransaction(mojom::kBitcoinMainnet, txid,
                               future.GetCallback());
  {
    auto result = future.Take();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->ToValue() == expected_tx.ToValue());
  }

  // Invalid value returned.
  url_loader_factory_.AddResponse(req_url, "some string");
  bitcoin_rpc_->GetTransaction(mojom::kBitcoinMainnet, txid,
                               future.GetCallback());
  ExpectError(future.Take(), ParsingError());

  // HTTP Error returned.
  url_loader_factory_.AddResponse(req_url, tx_json,
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  bitcoin_rpc_->GetTransaction(mojom::kBitcoinMainnet, txid,
                               future.GetCallback());
  ExpectError(future.Take(), InternalError());

  // Testnet works.
  url_loader_factory_.ClearResponses();
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "tx/" + txid, tx_json);
  bitcoin_rpc_->GetTransaction(mojom::kBitcoinTestnet, txid,
                               future.GetCallback());
  {
    auto result = future.Take();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->ToValue() == expected_tx.ToValue());
  }

  // Invalid chain fails.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetTransaction("0x123", txid, future.GetCallback());
  ExpectError(future.Take(), InternalError());

  // Invalid txid arg format fails.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetTransaction(mojom::kBitcoinMainnet, txid + "/",
                               future.GetCallback());
  ExpectError(future.Take(), InternalError());
}

TEST_F(BitcoinRpcUnitTest, GetTransactionRaw) {
  using GetTransactionRawResult =
      base::expected<std::vector<uint8_t>, std::string>;
  base::test::TestFuture<GetTransactionRawResult> future;

  const std::string txid =
      "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5";
  const std::string req_url = mainnet_rpc_url_ + "tx/" + txid + "/hex";

  const std::string tx_json = R"(010203)";

  std::vector<uint8_t> tx_expected = {1, 2, 3};

  // GetTransaction works.
  url_loader_factory_.AddResponse(req_url, tx_json);
  bitcoin_rpc_->GetTransactionRaw(mojom::kBitcoinMainnet, txid,
                                  future.GetCallback());
  EXPECT_EQ(future.Take(), GetTransactionRawResult(base::ok(tx_expected)));

  // Invalid value returned.
  url_loader_factory_.AddResponse(req_url, "some string");
  bitcoin_rpc_->GetTransactionRaw(mojom::kBitcoinMainnet, txid,
                                  future.GetCallback());
  ExpectError(future.Take(), ParsingError());

  // HTTP Error returned.
  url_loader_factory_.AddResponse(req_url, tx_json,
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  bitcoin_rpc_->GetTransactionRaw(mojom::kBitcoinMainnet, txid,
                                  future.GetCallback());
  ExpectError(future.Take(), InternalError());

  // Testnet works.
  url_loader_factory_.ClearResponses();
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "tx/" + txid + "/hex",
                                  tx_json);
  bitcoin_rpc_->GetTransactionRaw(mojom::kBitcoinTestnet, txid,
                                  future.GetCallback());
  EXPECT_EQ(future.Take(), GetTransactionRawResult(base::ok(tx_expected)));

  // Invalid chain fails.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetTransactionRaw("0x123", txid, future.GetCallback());
  ExpectError(future.Take(), InternalError());

  // Invalid txid arg format fails.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetTransactionRaw(mojom::kBitcoinMainnet, txid + "/",
                                  future.GetCallback());
  ExpectError(future.Take(), InternalError());
}

TEST_F(BitcoinRpcUnitTest, GetAddressStats) {
  using GetAddressStatsResult =
      base::expected<bitcoin_rpc::AddressStats, std::string>;
  base::test::TestFuture<GetAddressStatsResult> future;

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
  url_loader_factory_.AddResponse(req_url, address_json);
  bitcoin_rpc_->GetAddressStats(mojom::kBitcoinMainnet, address,
                                future.GetCallback());
  {
    auto result = future.Take();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->ToValue() == stats.ToValue());
  }

  // Invalid value returned.
  url_loader_factory_.AddResponse(req_url, "[123]");
  bitcoin_rpc_->GetAddressStats(mojom::kBitcoinMainnet, address,
                                future.GetCallback());
  ExpectError(future.Take(), ParsingError());

  // HTTP Error returned.
  url_loader_factory_.AddResponse(req_url, address_json,
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  bitcoin_rpc_->GetAddressStats(mojom::kBitcoinMainnet, address,
                                future.GetCallback());
  ExpectError(future.Take(), InternalError());

  // Testnet works.
  url_loader_factory_.ClearResponses();
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "address/" + address,
                                  address_json);
  bitcoin_rpc_->GetAddressStats(mojom::kBitcoinTestnet, address,
                                future.GetCallback());
  {
    auto result = future.Take();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->ToValue() == stats.ToValue());
  }

  // Invalid chain fails.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetAddressStats("0x123", address, future.GetCallback());
  ExpectError(future.Take(), InternalError());

  // Invalid address arg format fails.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetAddressStats(mojom::kBitcoinMainnet, address + "/",
                                future.GetCallback());
  ExpectError(future.Take(), InternalError());
}

TEST_F(BitcoinRpcUnitTest, GetUtxoList) {
  using GetUtxoListResult =
      base::expected<bitcoin_rpc::UnspentOutputs, std::string>;
  base::test::TestFuture<GetUtxoListResult> future;

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

  bitcoin_rpc::UnspentOutputs utxos;
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

  auto expect_utxos = [&](const GetUtxoListResult& result) {
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2u);
    EXPECT_TRUE((*result)[0].ToValue() == utxos[0].ToValue());
    EXPECT_TRUE((*result)[1].ToValue() == utxos[1].ToValue());
  };

  // GetUtxoList works.
  url_loader_factory_.AddResponse(req_url, utxo_json);
  bitcoin_rpc_->GetUtxoList(mojom::kBitcoinMainnet, address,
                            future.GetCallback());
  expect_utxos(future.Take());

  // Invalid value returned.
  url_loader_factory_.AddResponse(req_url, "[123]");
  bitcoin_rpc_->GetUtxoList(mojom::kBitcoinMainnet, address,
                            future.GetCallback());
  ExpectError(future.Take(), ParsingError());

  // HTTP Error returned.
  url_loader_factory_.AddResponse(req_url, utxo_json,
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  bitcoin_rpc_->GetUtxoList(mojom::kBitcoinMainnet, address,
                            future.GetCallback());
  ExpectError(future.Take(), InternalError());

  // Testnet works.
  url_loader_factory_.ClearResponses();
  url_loader_factory_.AddResponse(
      testnet_rpc_url_ + "address/" + address + "/utxo", utxo_json);
  bitcoin_rpc_->GetUtxoList(mojom::kBitcoinTestnet, address,
                            future.GetCallback());
  expect_utxos(future.Take());

  // Invalid chain fails.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetUtxoList("0x123", address, future.GetCallback());
  ExpectError(future.Take(), InternalError());

  // Invalid address arg format fails.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->GetUtxoList(mojom::kBitcoinMainnet, address + "/",
                            future.GetCallback());
  ExpectError(future.Take(), InternalError());
}

TEST_F(BitcoinRpcUnitTest, PostTransaction) {
  using PostTransactionResult = base::expected<std::string, std::string>;
  base::test::TestFuture<PostTransactionResult> future;

  const std::string req_url = mainnet_rpc_url_ + "tx";
  const std::string txid =
      "1fca84164f59606710ff4cf0fd660753bd299e30bb2c8194117fdb965ace67b9";

  // PostTransaction works.
  bitcoin_rpc_->PostTransaction(mojom::kBitcoinMainnet, {1, 2, 3},
                                future.GetCallback());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return url_loader_factory_.pending_requests()->size() == 1u; }));
  auto request = url_loader_factory_.GetPendingRequest(0)->request;
  EXPECT_EQ(request.url, req_url);
  EXPECT_EQ(request.request_body->elements()
                ->at(0)
                .As<network::DataElementBytes>()
                .AsStringPiece(),
            "010203");
  url_loader_factory_.AddResponse(req_url, txid);
  EXPECT_EQ(future.Take(), PostTransactionResult(base::ok(txid)));

  // Invalid value returned.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->PostTransaction(mojom::kBitcoinMainnet, {1, 2, 3},
                                future.GetCallback());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return url_loader_factory_.pending_requests()->size() == 1u; }));
  request = url_loader_factory_.GetPendingRequest(0)->request;
  EXPECT_EQ(request.url, req_url);
  url_loader_factory_.AddResponse(req_url, "not valid txid");
  ExpectError(future.Take(), ParsingError());

  // HTTP Error returned.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->PostTransaction(mojom::kBitcoinMainnet, {1, 2, 3},
                                future.GetCallback());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return url_loader_factory_.pending_requests()->size() == 1u; }));
  request = url_loader_factory_.GetPendingRequest(0)->request;
  EXPECT_EQ(request.url, req_url);
  url_loader_factory_.AddResponse(req_url, txid,
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  ExpectError(future.Take(), InternalError());

  // Testnet works.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->PostTransaction(mojom::kBitcoinTestnet, {1, 2, 3},
                                future.GetCallback());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return url_loader_factory_.pending_requests()->size() == 1u; }));
  request = url_loader_factory_.GetPendingRequest(0)->request;
  EXPECT_EQ(request.url, testnet_rpc_url_ + "tx");
  EXPECT_EQ(request.request_body->elements()
                ->at(0)
                .As<network::DataElementBytes>()
                .AsStringPiece(),
            "010203");
  url_loader_factory_.AddResponse(testnet_rpc_url_ + "tx", txid);
  EXPECT_EQ(future.Take(), PostTransactionResult(base::ok(txid)));

  // Invalid chain fails.
  url_loader_factory_.ClearResponses();
  bitcoin_rpc_->PostTransaction("0x123", {1, 2, 3}, future.GetCallback());
  ExpectError(future.Take(), InternalError());
}

}  // namespace brave_wallet
