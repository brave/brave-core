/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_ratio_service.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

void OnGetPrice(bool* callback_run,
                bool expected_success,
                std::vector<brave_wallet::mojom::AssetPricePtr> expected_values,
                bool success,
                std::vector<brave_wallet::mojom::AssetPricePtr> values) {
  EXPECT_EQ(expected_success, success);
  EXPECT_EQ(expected_values, values);
  *callback_run = true;
}

void OnGetPriceHistory(
    bool* callback_run,
    bool expected_success,
    std::vector<brave_wallet::mojom::AssetTimePricePtr> expected_values,
    bool success,
    std::vector<brave_wallet::mojom::AssetTimePricePtr> values) {
  EXPECT_EQ(expected_success, success);
  EXPECT_EQ(expected_values, values);
  *callback_run = true;
}

void OnGetCoinMarkets(
    bool* callback_run,
    bool expected_success,
    std::vector<brave_wallet::mojom::CoinMarketPtr> expected_values,
    bool success,
    std::vector<brave_wallet::mojom::CoinMarketPtr> values) {
  EXPECT_EQ(expected_success, success);
  EXPECT_EQ(expected_values, values);
  *callback_run = true;
}

}  // namespace

namespace brave_wallet {

class AssetRatioServiceUnitTest : public testing::Test {
 public:
  AssetRatioServiceUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    asset_ratio_service_ =
        std::make_unique<AssetRatioService>(shared_url_loader_factory_);
  }

  // Helper function to create AssetPriceRequest
  std::vector<mojom::AssetPriceRequestPtr> CreatePriceRequests() {
    std::vector<mojom::AssetPriceRequestPtr> requests;

    // ETH token (BAT)
    auto eth_request = mojom::AssetPriceRequest::New();
    eth_request->coin = mojom::CoinType::ETH;
    eth_request->chain_id = "0x1";
    eth_request->address = "0x0D8775F648430679A709E98d2b0Cb6250d2887EF";
    requests.push_back(std::move(eth_request));

    // SOL token (USDC)
    auto sol_request = mojom::AssetPriceRequest::New();
    sol_request->coin = mojom::CoinType::SOL;
    sol_request->chain_id = "0x65";
    sol_request->address = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
    requests.push_back(std::move(sol_request));

    // BTC native token
    auto btc_request = mojom::AssetPriceRequest::New();
    btc_request->coin = mojom::CoinType::BTC;
    btc_request->chain_id = "bitcoin_mainnet";
    requests.push_back(std::move(btc_request));

    return requests;
  }

  ~AssetRatioServiceUnitTest() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetErrorInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content,
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
  }

  void TestGetBuyUrlV1(mojom::OnRampProvider on_ramp_provider,
                       const std::string& chain_id,
                       const std::string& address,
                       const std::string& symbol,
                       const std::string& amount,
                       const std::string& currency_code,
                       const std::string& expected_url,
                       std::optional<std::string> expected_error) {
    base::RunLoop run_loop;
    asset_ratio_service_->GetBuyUrlV1(
        on_ramp_provider, chain_id, address, symbol, amount, currency_code,
        base::BindLambdaForTesting(
            [&](const std::string& url,
                const std::optional<std::string>& error) {
              EXPECT_EQ(url, expected_url);
              EXPECT_EQ(error, expected_error);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetSellUrl(mojom::OffRampProvider off_ramp_provider,
                      const std::string& chain_id,
                      const std::string& symbol,
                      const std::string& amount,
                      const std::string& currency_code,
                      const std::string& expected_url,
                      std::optional<std::string> expected_error) {
    base::RunLoop run_loop;
    asset_ratio_service_->GetSellUrl(
        off_ramp_provider, chain_id, symbol, amount, currency_code,
        base::BindLambdaForTesting(
            [&](const std::string& url,
                const std::optional<std::string>& error) {
              EXPECT_EQ(url, expected_url);
              EXPECT_EQ(error, expected_error);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

 protected:
  std::unique_ptr<AssetRatioService> asset_ratio_service_;
  base::test::TaskEnvironment task_environment_;

 private:
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(AssetRatioServiceUnitTest, GetBuyUrlV1Ramp) {
  TestGetBuyUrlV1(mojom::OnRampProvider::kRamp, mojom::kMainnetChainId,
                  "0xdeadbeef", "USDC", "55000000", "USD",
                  "https://app.ramp.network/"
                  "?enabledFlows=ONRAMP"
                  "&userAddress=0xdeadbeef&swapAsset=USDC&fiatValue=55000000"
                  "&fiatCurrency=USD&hostApiKey="
                  "8yxja8782as5essk2myz3bmh4az6gpq4nte9n2gf",
                  std::nullopt);
}

TEST_F(AssetRatioServiceUnitTest, GetSellUrl) {
  TestGetSellUrl(mojom::OffRampProvider::kRamp, mojom::kMainnetChainId,
                 "ETH_BAT", "250", "USD",
                 "https://app.ramp.network/"
                 "?enabledFlows=OFFRAMP"
                 "&swapAsset=ETH_BAT&offrampAsset=ETH_BAT"
                 "&swapAmount=250"
                 "&fiatCurrency=USD&hostApiKey="
                 "y57zqta99ohs7o2paf4ak6vpfb7wf8ubj9krwtwe",
                 std::nullopt);
}

TEST_F(AssetRatioServiceUnitTest, GetPrice) {
  SetInterceptor(R"([
    {
      "coin": "ETH",
      "chain_id": "0x1",
      "address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
      "price": "0.55393",
      "percentage_change_24h": "0.012",
      "vs_currency": "USD",
      "cache_status": "HIT",
      "source": "coingecko"
    },
    {
      "coin": "SOL",
      "chain_id": "0x65",
      "address": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
      "price": "0.998",
      "percentage_change_24h": "0.02",
      "vs_currency": "USD",
      "cache_status": "MISS",
      "source": "jupiter"
    },
    {
      "coin": "BTC",
      "chain_id": "bitcoin_mainnet",
      "price": "102000",
      "percentage_change_24h": "5.2",
      "vs_currency": "USD",
      "cache_status": "MISS",
      "source": "coingecko"
    }
  ])");

  std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response;

  // ETH token (BAT)
  auto asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->coin = mojom::CoinType::ETH;
  asset_price->chain_id = "0x1";
  asset_price->address = "0x0D8775F648430679A709E98d2b0Cb6250d2887EF";
  asset_price->price = "0.55393";
  asset_price->vs_currency = "USD";
  asset_price->cache_status = mojom::Gate3CacheStatus::kHit;
  asset_price->source = mojom::AssetPriceSource::kCoingecko;
  asset_price->percentage_change_24h = "0.012";
  expected_prices_response.push_back(std::move(asset_price));

  // SOL token (USDC)
  asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->coin = mojom::CoinType::SOL;
  asset_price->chain_id = "0x65";
  asset_price->address = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
  asset_price->price = "0.998";
  asset_price->vs_currency = "USD";
  asset_price->cache_status = mojom::Gate3CacheStatus::kMiss;
  asset_price->source = mojom::AssetPriceSource::kJupiter;
  asset_price->percentage_change_24h = "0.02";
  expected_prices_response.push_back(std::move(asset_price));

  // BTC native token
  asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->coin = mojom::CoinType::BTC;
  asset_price->chain_id = "bitcoin_mainnet";
  asset_price->price = "102000";
  asset_price->vs_currency = "USD";
  asset_price->cache_status = mojom::Gate3CacheStatus::kMiss;
  asset_price->source = mojom::AssetPriceSource::kCoingecko;
  asset_price->percentage_change_24h = "5.2";
  expected_prices_response.push_back(std::move(asset_price));

  bool callback_run = false;
  auto requests = CreatePriceRequests();
  asset_ratio_service_->GetPrice(
      std::move(requests), "USD",
      base::BindOnce(&OnGetPrice, &callback_run, true,
                     std::move(expected_prices_response)));

  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioServiceUnitTest, GetPriceError) {
  std::string error = "error";
  SetErrorInterceptor(error);
  std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response;
  bool callback_run = false;
  auto requests = CreatePriceRequests();
  asset_ratio_service_->GetPrice(
      std::move(requests), "USD",
      base::BindOnce(&OnGetPrice, &callback_run, false,
                     std::move(expected_prices_response)));

  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioServiceUnitTest, GetPriceUnexpectedResponse) {
  SetInterceptor("expecto patronum");
  std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response;
  bool callback_run = false;
  auto requests = CreatePriceRequests();
  asset_ratio_service_->GetPrice(
      std::move(requests), "USD",
      base::BindOnce(&OnGetPrice, &callback_run, false,
                     std::move(expected_prices_response)));

  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioServiceUnitTest, GetPriceHistory) {
  SetInterceptor(R"({
      "payload": {
        "prices":[[1622733088498,0.8201346624954003],[1622737203757,0.8096978545029869]],
        "market_caps":[[1622733088498,1223507820.383275],[1622737203757,1210972881.4928021]],
        "total_volumes":[[1622733088498,163426828.00299588],[1622737203757,157618689.0971025]]
      }
    })");

  std::vector<brave_wallet::mojom::AssetTimePricePtr>
      expected_price_history_response;

  auto asset_time_price = brave_wallet::mojom::AssetTimePrice::New();
  asset_time_price->date = base::Milliseconds(1622733088498);
  asset_time_price->price = "0.8201346624954003";
  expected_price_history_response.push_back(std::move(asset_time_price));

  asset_time_price = brave_wallet::mojom::AssetTimePrice::New();
  asset_time_price->date = base::Milliseconds(1622737203757);
  asset_time_price->price = "0.8096978545029869";
  expected_price_history_response.push_back(std::move(asset_time_price));

  bool callback_run = false;
  asset_ratio_service_->GetPriceHistory(
      "bat", "usd", brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPriceHistory, &callback_run, true,
                     std::move(expected_price_history_response)));

  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioServiceUnitTest, GetPriceHistoryError) {
  std::string error = "error";
  SetErrorInterceptor(error);
  std::vector<brave_wallet::mojom::AssetTimePricePtr>
      expected_price_history_response;
  bool callback_run = false;
  asset_ratio_service_->GetPriceHistory(
      "bat", "usd", brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPriceHistory, &callback_run, false,
                     std::move(expected_price_history_response)));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioServiceUnitTest, GetPriceHistoryUnexpectedResponse) {
  SetInterceptor("Accio!");
  std::vector<brave_wallet::mojom::AssetTimePricePtr>
      expected_price_history_response;

  bool callback_run = false;
  asset_ratio_service_->GetPriceHistory(
      "bat", "usd", brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPriceHistory, &callback_run, false,
                     std::move(expected_price_history_response)));

  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioServiceUnitTest, GetPriceHistoryURL) {
  // Basic test
  EXPECT_EQ("/v2/history/coingecko/bat/usd/1d",
            AssetRatioService::GetPriceHistoryURL(
                "bat", "usd", brave_wallet::mojom::AssetPriceTimeframe::OneDay)
                .path());
  // Test the remaining timeframes
  EXPECT_EQ("/v2/history/coingecko/eth/cad/live",
            AssetRatioService::GetPriceHistoryURL(
                "eth", "cad", brave_wallet::mojom::AssetPriceTimeframe::Live)
                .path());
  EXPECT_EQ("/v2/history/coingecko/eth/cad/1w",
            AssetRatioService::GetPriceHistoryURL(
                "eth", "cad", brave_wallet::mojom::AssetPriceTimeframe::OneWeek)
                .path());
  EXPECT_EQ(
      "/v2/history/coingecko/eth/cad/1m",
      AssetRatioService::GetPriceHistoryURL(
          "eth", "cad", brave_wallet::mojom::AssetPriceTimeframe::OneMonth)
          .path());
  EXPECT_EQ(
      "/v2/history/coingecko/eth/cad/3m",
      AssetRatioService::GetPriceHistoryURL(
          "eth", "cad", brave_wallet::mojom::AssetPriceTimeframe::ThreeMonths)
          .path());
  EXPECT_EQ("/v2/history/coingecko/eth/cad/1y",
            AssetRatioService::GetPriceHistoryURL(
                "eth", "cad", brave_wallet::mojom::AssetPriceTimeframe::OneYear)
                .path());
  EXPECT_EQ("/v2/history/coingecko/eth/cad/all",
            AssetRatioService::GetPriceHistoryURL(
                "eth", "cad", brave_wallet::mojom::AssetPriceTimeframe::All)
                .path());
}

TEST_F(AssetRatioServiceUnitTest, GetCoinMarkets) {
  SetInterceptor(R"({
      "payload": [
        {
          "id": "bitcoin",
          "symbol": "btc",
          "name": "Bitcoin",
          "image": "https://assets.coingecko.com/coins/images/1/large/bitcoin.png?1547033579",
          "market_cap": 727960800075,
          "market_cap_rank": 1,
          "current_price": 38357,
          "price_change_24h": -1229.64683216549,
          "price_change_percentage_24h": -3.10625,
          "total_volume": 17160995925
        }
      ]
    })");
  std::vector<brave_wallet::mojom::CoinMarketPtr>
      expected_coin_markets_response;

  auto coin_market = brave_wallet::mojom::CoinMarket::New();
  coin_market->id = "bitcoin";
  coin_market->symbol = "btc";
  coin_market->name = "Bitcoin";
  coin_market->image =
      "https://assets.coingecko.com/coins/images/1/large/"
      "bitcoin.png?1547033579";
  coin_market->market_cap = 727960800075;
  coin_market->market_cap_rank = 1;
  coin_market->current_price = 38357;
  coin_market->price_change_24h = -1229.64683216549;
  coin_market->price_change_percentage_24h = -3.10625;
  coin_market->total_volume = 17160995925;
  expected_coin_markets_response.push_back(std::move(coin_market));

  bool callback_run = false;
  asset_ratio_service_->GetCoinMarkets(
      "usd", 100,
      base::BindOnce(&OnGetCoinMarkets, &callback_run, true,
                     std::move(expected_coin_markets_response)));

  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioServiceUnitTest, GetCoinMarketsUnexpectedResponse) {
  SetInterceptor("wingardium leviosa");
  std::vector<brave_wallet::mojom::CoinMarketPtr>
      expected_coin_markets_response;

  bool callback_run = false;
  asset_ratio_service_->GetCoinMarkets(
      "bat", 99,
      base::BindOnce(&OnGetCoinMarkets, &callback_run, false,
                     std::move(expected_coin_markets_response)));

  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioServiceUnitTest, GetStripeBuyURL) {
  SetInterceptor(R"({
      "url": "https://crypto.link.com?session_hash=abcdefgh"
    })");

  TestGetBuyUrlV1(mojom::OnRampProvider::kStripe, mojom::kMainnetChainId,
                  "0xdeadbeef", "USDC", "55000000", "USD",
                  "https://crypto.link.com?session_hash=abcdefgh",
                  std::nullopt);

  // Test with unexpected response
  SetInterceptor("mischief managed");
  TestGetBuyUrlV1(mojom::OnRampProvider::kStripe, mojom::kMainnetChainId,
                  "0xdeadbeef", "USDC", "55000000", "USD", "", "PARSING_ERROR");

  // Test with non 2XX response
  SetErrorInterceptor("");
  TestGetBuyUrlV1(mojom::OnRampProvider::kStripe, mojom::kMainnetChainId,
                  "0xdeadbeef", "USDC", "55000000", "USD", "",
                  "INTERNAL_SERVICE_ERROR");
}

TEST_F(AssetRatioServiceUnitTest, GetBuyUrlV1Coinbase) {
  // Eth address
  TestGetBuyUrlV1(
      mojom::OnRampProvider::kCoinbase, mojom::kMainnetChainId,
      "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961", "USDC", "1", "USD",
      "https://pay.coinbase.com/"
      "?appId=8072ff71-8469-4fef-9404-7c905e2359c9&defaultExperience=buy&"
      "presetFiatAmount=1&destinationWallets=%5B%7B%22address%22%3A%"
      "220xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961%22%2C%22assets%22%3A%5B%"
      "22USDC%22%5D%2C%22blockchains%22%3A%5B%22ethereum%22%2C%22arbitrum%22%"
      "2C%22optimism%22%2C%22polygon%22%2C%22avalanche-c-chain%22%2C%22celo%22%"
      "5D%7D%5D",
      std::nullopt);

  // Sol address
  TestGetBuyUrlV1(
      mojom::OnRampProvider::kCoinbase, mojom::kMainnetChainId,
      "FBG2vwk2tGKHbEWHSxf7rJGDuZ2eHaaNQ8u6c7xGt9Yv", "SOL", "1", "USD",
      "https://pay.coinbase.com/"
      "?appId=8072ff71-8469-4fef-9404-7c905e2359c9&defaultExperience=buy&"
      "presetFiatAmount=1&destinationWallets=%5B%7B%22address%22%3A%"
      "22FBG2vwk2tGKHbEWHSxf7rJGDuZ2eHaaNQ8u6c7xGt9Yv%22%2C%22assets%22%3A%5B%"
      "22SOL%22%5D%2C%22blockchains%22%3A%5B%22solana%22%5D%7D%5D",
      std::nullopt);
}

}  // namespace brave_wallet
