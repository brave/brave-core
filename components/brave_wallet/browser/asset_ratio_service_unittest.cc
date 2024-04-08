/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_ratio_service.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/containers/contains.h"
#include "base/functional/callback_forward.h"
#include "base/ranges/algorithm.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "gtest/gtest.h"
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

  ~AssetRatioServiceUnitTest() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }

  void SetInterceptor(const std::string& content,
                      const std::string expected_header = "") {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content, expected_header](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          std::string header;
          request.headers.GetHeader("Authorization", &header);
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

  void TestGetServiceProvider(
      const std::string& content,
      const std::string& countries,
      const std::string& from_assets,
      const std::string& to_assets,
      const std::string& service_providers,
      const std::string& payment_method_types,
      const std::string& statuses,
      AssetRatioService::GetServiceProvidersCallback callback,
      const bool error_interceptor = false) {
    if(!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetServiceProviders(
        countries, from_assets, to_assets, service_providers, payment_method_types, statuses,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::ServiceProviderPtr> sps,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(sps), errors);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetCryptoQuotes(const std::string& content,
                           const std::string& country,
                           const std::string& from_asset,
                           const std::string& to_asset,
                           const double source_amount,
                           const std::string& account,
                           AssetRatioService::GetCryptoQuotesCallback callback,
                           const bool error_interceptor = false) {
    if (!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetCryptoQuotes(
        country, from_asset, to_asset, source_amount, account,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::CryptoQuotePtr> quotes, const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(quotes), errors);
              run_loop.Quit();
            }));
    run_loop.Run();    
  }

  void TestGetPaymentMethods(const std::string& content,
                             const std::string& countries,
                             const std::string& fiat_currencies,
                             const std::string& crypto_currencies,
                             const std::string& service_providers,
                             const std::string& payment_method_types,
                             const std::string& statuses,
                             AssetRatioService::GetPaymentMethodsCallback callback,
                             const bool error_interceptor = false) {
    if (!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetPaymentMethods(
        countries, fiat_currencies, crypto_currencies, service_providers, payment_method_types, statuses,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::PaymentMethodPtr> payment_methods, const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(payment_methods), errors);
              run_loop.Quit();
            }));
    run_loop.Run();    

  }

  void TestGetFiatCurrencies(const std::string& content,
                             const std::string& countries,
                             const std::string& fiat_currencies,
                             const std::string& crypto_currencies,
                             const std::string& service_providers,
                             const std::string& payment_method_types,
                             const std::string& statuses,
                             AssetRatioService::GetFiatCurrenciesCallback callback,
                             const bool error_interceptor = false) {
    if (!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetFiatCurrencies(
        countries, fiat_currencies, crypto_currencies, service_providers, payment_method_types, statuses,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::FiatCurrencyPtr> fiat_currencies, const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(fiat_currencies), errors);
              run_loop.Quit();
            }));
    run_loop.Run();    

  }

  void TestGetCryptoCurrencies(
      const std::string& content,
      const std::string& countries,
      const std::string& fiat_currencies,
      const std::string& crypto_currencies,
      const std::string& service_providers,
      const std::string& payment_method_types,
      const std::string& statuses,
      AssetRatioService::GetCryptoCurrenciesCallback callback,
      const bool error_interceptor = false) {
    if (!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetCryptoCurrencies(
        countries, fiat_currencies, crypto_currencies, service_providers,
        payment_method_types, statuses,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::CryptoCurrencyPtr> crypto_currencies,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(crypto_currencies), errors);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetCountries(const std::string& content,
                        const std::string& countries,
                        const std::string& fiat_currencies,
                        const std::string& crypto_currencies,
                        const std::string& service_providers,
                        const std::string& payment_method_types,
                        const std::string& statuses,
                        AssetRatioService::GetCountriesCallback callback,
                        const bool error_interceptor = false) {
    if (!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetCountries(
        countries, fiat_currencies, crypto_currencies, service_providers,
        payment_method_types, statuses,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::CountryPtr> countries,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(countries), errors);
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

TEST_F(AssetRatioServiceUnitTest, GetBuyUrlV1Sardine) {
  SetInterceptor(R"({
     "clientToken":"74618e17-a537-4f5d-ab4d-9916739560b1",
     "expiresAt":"2022-07-25T19:59:57Z"
    })");
  TestGetBuyUrlV1(mojom::OnRampProvider::kSardine, mojom::kMainnetChainId,
                  "0xdeadbeef", "USDC", "55000000", "USD",
                  "https://crypto.sardine.ai/"
                  "?address=0xdeadbeef&network=ethereum&asset_type=USDC&fiat_"
                  "amount=55000000&fiat_currency=USD&client_token=74618e17-"
                  "a537-4f5d-ab4d-9916739560b1&fixed_asset_type=USDC&fixed_"
                  "network=ethereum",
                  std::nullopt);

  // Timeout yields error
  std::string error = "error";
  SetErrorInterceptor(error);
  TestGetBuyUrlV1(mojom::OnRampProvider::kSardine, "ethereum", "0xdeadbeef",
                  "USDC", "55000000", "USD", "", "INTERNAL_SERVICE_ERROR");

  // Unexpected JSON response (empty body) yields error
  SetInterceptor(R"({})");
  TestGetBuyUrlV1(mojom::OnRampProvider::kSardine, "ethereum", "0xdeadbeef",
                  "USDC", "55000000", "USD", "", "INTERNAL_SERVICE_ERROR");
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
  SetInterceptor(R"(
      {
         "payload":{
           "bat":{
             "btc":0.00001732,
             "btc_timeframe_change":8.021672460190562,
             "usd":0.55393,
             "usd_timeframe_change":9.523443444373276
           },
           "link":{
             "btc":0.00261901,
             "btc_timeframe_change":0.5871625385632929,
             "usd":83.77,
             "usd_timeframe_change":1.7646208048244043
           }
         },
         "lastUpdated":"2021-07-16T19:11:28.907Z"
       })");

  std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response;
  auto asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->from_asset = "bat";
  asset_price->to_asset = "btc";
  asset_price->price = "0.00001732";
  asset_price->asset_timeframe_change = "8.021672460190562";
  expected_prices_response.push_back(std::move(asset_price));

  asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->from_asset = "bat";
  asset_price->to_asset = "usd";
  asset_price->price = "0.55393";
  asset_price->asset_timeframe_change = "9.523443444373276";
  expected_prices_response.push_back(std::move(asset_price));

  asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->from_asset = "link";
  asset_price->to_asset = "btc";
  asset_price->price = "0.00261901";
  asset_price->asset_timeframe_change = "0.5871625385632929";
  expected_prices_response.push_back(std::move(asset_price));

  asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->from_asset = "link";
  asset_price->to_asset = "usd";
  asset_price->price = "83.77";
  asset_price->asset_timeframe_change = "1.7646208048244043";
  expected_prices_response.push_back(std::move(asset_price));

  bool callback_run = false;
  asset_ratio_service_->GetPrice(
      {"bat", "link"}, {"btc", "usd"},
      brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPrice, &callback_run, true,
                     std::move(expected_prices_response)));

  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioServiceUnitTest, GetPriceUppercase) {
  SetInterceptor(R"(
       {
         "payload":{
           "bat":{
             "btc":0.00001732,
             "btc_timeframe_change":8.021672460190562
           }
         },
         "lastUpdated":"2021-07-16T19:11:28.907Z"
       })");

  std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response;
  auto asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->from_asset = "bat";
  asset_price->to_asset = "btc";
  asset_price->price = "0.00001732";
  asset_price->asset_timeframe_change = "8.021672460190562";
  expected_prices_response.push_back(std::move(asset_price));

  bool callback_run = false;
  asset_ratio_service_->GetPrice(
      {"BAT"}, {"BTC"}, brave_wallet::mojom::AssetPriceTimeframe::OneDay,
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
  asset_ratio_service_->GetPrice(
      {"bat"}, {"btc"}, brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPrice, &callback_run, false,
                     std::move(expected_prices_response)));

  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioServiceUnitTest, GetPriceUnexpectedResponse) {
  SetInterceptor("expecto patronum");
  std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response;
  bool callback_run = false;
  asset_ratio_service_->GetPrice(
      {"bat"}, {"btc"}, brave_wallet::mojom::AssetPriceTimeframe::OneDay,
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

TEST_F(AssetRatioServiceUnitTest, GetServiceProviders) {
  const auto url = AssetRatioService::GetServiceProviderURL(
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "");
  EXPECT_EQ(url.path(), "/service-providers");
  EXPECT_EQ(url.query(),
            "accountFilter=false&statuses=LIVE%2CRECENTLY_ADDED&countries=US%"
            "2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies=BTC%2CETH&"
            "serviceProviders=BANXA%2CBLOCKCHAINDOTCOM&paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");

  TestGetServiceProvider(
    R"([
  {
    "serviceProvider": "BANXA",
    "name": "Banxa",
    "status": "LIVE",
    "categories": [
      "CRYPTO_ONRAMP"
    ],
    "categoryStatuses": {
      "CRYPTO_ONRAMP": "LIVE"
    },
    "websiteUrl": "http://www.banxa.com",
    "logos": {
      "dark": "https://images-serviceprovider.meld.io/BANXA/logo_dark.png",
      "light": "https://images-serviceprovider.meld.io/BANXA/logo_light.png",
      "darkShort": "https://images-serviceprovider.meld.io/BANXA/short_logo_dark.png",
      "lightShort": "https://images-serviceprovider.meld.io/BANXA/short_logo_light.png"
    }
  },
  {
    "serviceProvider": "BLOCKCHAINDOTCOM",
    "name": "Blockchain.com",
    "status": "LIVE",
    "categories": [
      "CRYPTO_ONRAMP"
    ],
    "categoryStatuses": {
      "CRYPTO_ONRAMP": "LIVE"
    },
    "websiteUrl": "https://www.blockchain.com",
    "logos": {
      "dark": "https://images-serviceprovider.meld.io/BLOCKCHAINDOTCOM/logo_dark.png",
      "light": "https://images-serviceprovider.meld.io/BLOCKCHAINDOTCOM/logo_light.png",
      "darkShort": "https://images-serviceprovider.meld.io/BLOCKCHAINDOTCOM/short_logo_dark.png",
      "lightShort": "https://images-serviceprovider.meld.io/BLOCKCHAINDOTCOM/short_logo_light.png"
    }
  }])", "US", "USD", "ETH", "", "", "",
      base::BindLambdaForTesting([&](std::vector<mojom::ServiceProviderPtr> sps,
                                     const std::optional<std::vector<std::string>>& errors) {
        EXPECT_FALSE(errors.has_value());
        EXPECT_EQ(base::ranges::count_if(sps, [](const auto& item){
          return item->name == "Banxa" && 
            item->service_provider == "BANXA" &&
            item->status == "LIVE" &&
            item->web_site_url == "http://www.banxa.com" &&
            !item->logo_images.empty();
        }), 1);
        EXPECT_EQ(base::ranges::count_if(sps, [](const auto& item){
          return item->name == "Blockchain.com" && 
          item->service_provider == "BLOCKCHAINDOTCOM" &&
          item->status == "LIVE" &&
            item->web_site_url == "https://www.blockchain.com" &&
          !item->logo_images.empty();}), 1);
      }));

  TestGetServiceProvider("some wrone data", "US", "USD", "ETH", "", "", "",
      base::BindLambdaForTesting([&](std::vector<mojom::ServiceProviderPtr> sps,
                                     const std::optional<std::vector<std::string>>& errors) {
        EXPECT_TRUE(errors.has_value());
        EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
      }));
  TestGetServiceProvider("some wrone data", "US", "USD", "ETH", "", "", "",
      base::BindLambdaForTesting([&](std::vector<mojom::ServiceProviderPtr> sps,
                                     const std::optional<std::vector<std::string>>& errors) {
        EXPECT_TRUE(errors.has_value());
        EXPECT_EQ(*errors, std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
      }), true);

  TestGetServiceProvider(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })", "US", "USD", "ETH", "", "", "",
  base::BindLambdaForTesting([&](std::vector<mojom::ServiceProviderPtr> sps,
                                  const std::optional<std::vector<std::string>>& errors) {
    EXPECT_TRUE(errors.has_value());
    EXPECT_EQ(*errors, 
      std::vector<std::string>({"[sourceAmount] must not be null", "[sourceCurrencyCode] must not be blank"})
    );
  }), false);
}

TEST_F(AssetRatioServiceUnitTest, GetCryptoQuotes) {
  TestGetCryptoQuotes(
      R"({
  "quotes": [
    {
      "transactionType": "CRYPTO_PURCHASE",
      "sourceAmount": 50,
      "sourceAmountWithoutFees": 43.97,
      "fiatAmountWithoutFees": 43.97,
      "destinationAmountWithoutFees": null,
      "sourceCurrencyCode": "USD",
      "countryCode": "US",
      "totalFee": 6.03,
      "networkFee": 3.53,
      "transactionFee": 2,
      "destinationAmount": 0.00066413,
      "destinationCurrencyCode": "BTC",
      "exchangeRate": 75286,
      "paymentMethodType": "APPLE_PAY",
      "customerScore": 20,
      "serviceProvider": "TRANSAK"
    }
  ],
  "message": null,
  "error": null
})",
      "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoQuotePtr> quotes,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          quotes,
                          [](const auto& item) {
                            return item->transaction_type == "CRYPTO_PURCHASE" &&
                              item->exchange_rate == 75286 &&
                              item->source_amount == 50 &&
                              item->source_amount_without_fee == 43.97 &&
                              item->total_fee == 6.03 &&
                              item->payment_method == "APPLE_PAY" &&
                              item->destination_amount == 0.00066413 &&
                              item->service_provider_id == "TRANSAK";  
                          }),
                      1);
          }));

  TestGetCryptoQuotes("some wrong data",
      "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoQuotePtr> quotes,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
          }));

  TestGetCryptoQuotes("some wrong data",
      "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoQuotePtr> quotes,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
          }), true);

  TestGetCryptoQuotes(
      R"({
  "quotes": [
    {
      "transactionType": "CRYPTO_PURCHASE",
      "sourceAmount": 50,
      "sourceAmountWithoutFees": 43.97,
      "fiatAmountWithoutFees": 43.97,
      "destinationAmountWithoutFees": null,
      "sourceCurrencyCode": "USD",
      "countryCode": "US",
      "totalFee": 6.03,
      "networkFee": 3.53,
      "transactionFee": 2,
      "destinationAmount": 0.00066413,
      "destinationCurrencyCode": "BTC",
      "exchangeRate": 75286,
      "paymentMethodType": "APPLE_PAY",
      "customerScore": 20,
      "serviceProvider": "TRANSAK"
    }
  ],
  "message": null,
  "error": "error description"
})",
      "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoQuotePtr> quotes,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"error description"});
            EXPECT_EQ(base::ranges::count_if(
                          quotes,
                          [](const auto& item) {
                            return item->transaction_type == "CRYPTO_PURCHASE" &&
                              item->exchange_rate == 75286 &&
                              item->source_amount == 50 &&
                              item->source_amount_without_fee == 43.97 &&
                              item->total_fee == 6.03 &&
                              item->payment_method == "APPLE_PAY" &&
                              item->destination_amount == 0.00066413 &&
                              item->service_provider_id == "TRANSAK";  
                          }),
                      1);
          }));

TestGetCryptoQuotes(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })", "US", "USD", "BTC", 50, "btc account address",
  base::BindLambdaForTesting([&](std::vector<mojom::CryptoQuotePtr> quotes,
                                  const std::optional<std::vector<std::string>>& errors) {
    EXPECT_TRUE(errors.has_value());
    EXPECT_EQ(*errors, 
      std::vector<std::string>({"[sourceAmount] must not be null", "[sourceCurrencyCode] must not be blank"})
    );
  }), false);
}

TEST_F(AssetRatioServiceUnitTest, GetPaymentMethods) {
  const auto url = AssetRatioService::GetGetPaymentMethodsURL(
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "");
  EXPECT_EQ(url.path(), "/service-providers/properties/payment-methods");
  EXPECT_EQ(url.query(),
            "includeServiceProviderDetails=false&statuses=LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&serviceProviders=BANXA%2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");

  TestGetPaymentMethods(R"([
  {
    "paymentMethod": "ACH",
    "name": "ACH",
    "paymentType": "BANK_TRANSFER",
    "logos": {
      "dark": "https://images-paymentMethod.meld.io/ACH/logo_dark.png",
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])", "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
        base::BindLambdaForTesting(
          [](std::vector<mojom::PaymentMethodPtr> payment_methods,
             const std::optional<std::vector<std::string>>& errors){
              EXPECT_FALSE(errors.has_value());
              EXPECT_EQ(base::ranges::count_if(
                          payment_methods,
                          [](const auto& item) {
                            return item->payment_method == "ACH" &&
                              item->name == "ACH" &&
                              item->payment_type == "BANK_TRANSFER" &&
                              !item->logo_images.empty();  
                          }),
                      1);

             }));

  TestGetPaymentMethods("some wrong data",
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::PaymentMethodPtr> payment_methods,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
          }));

  TestGetPaymentMethods("some wrong data",
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::PaymentMethodPtr> payment_methods,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
          }), true);

  TestGetPaymentMethods(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })", "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
  base::BindLambdaForTesting([&](std::vector<mojom::PaymentMethodPtr> payment_methods,
                                  const std::optional<std::vector<std::string>>& errors) {
    EXPECT_TRUE(errors.has_value());
    EXPECT_EQ(*errors, 
      std::vector<std::string>({"[sourceAmount] must not be null", "[sourceCurrencyCode] must not be blank"})
    );
  }), false);          
}

TEST_F(AssetRatioServiceUnitTest, GetFiatCurrencies) {
  const auto url = AssetRatioService::GetFiatCurrenciesURL(
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "");
  EXPECT_EQ(url.path(), "/service-providers/properties/fiat-currencies");
  EXPECT_EQ(url.query(),
            "includeServiceProviderDetails=false&statuses=LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&serviceProviders=BANXA%2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");

  TestGetFiatCurrencies(
      R"([
  {
    "currencyCode": "AFN",
    "name": "Afghani",
    "symbolImageUrl": "https://images-currency.meld.io/fiat/AFN/symbol.png"
  },
  {
    "currencyCode": "DZD",
    "name": "Algerian Dinar",
    "symbolImageUrl": "https://images-currency.meld.io/fiat/DZD/symbol.png"
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::FiatCurrencyPtr> fiat_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          fiat_currencies,
                          [](const auto& item) {
                            return item->currency_code == "AFN" &&
                                   item->name == "Afghani" &&
                                   item->symbol_image_url ==
                                       "https://images-currency.meld.io/fiat/"
                                       "AFN/symbol.png";
                          }),
                      1);
            EXPECT_EQ(base::ranges::count_if(
                          fiat_currencies,
                          [](const auto& item) {
                            return item->currency_code == "DZD" &&
                                   item->name == "Algerian Dinar" &&
                                   item->symbol_image_url ==
                                       "https://images-currency.meld.io/fiat/"
                                       "DZD/symbol.png";
                          }),
                      1);
          }));

  TestGetFiatCurrencies(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::FiatCurrencyPtr> fiat_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
          }));

  TestGetFiatCurrencies(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::FiatCurrencyPtr> fiat_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
          }),
      true);

  TestGetFiatCurrencies(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [&](std::vector<mojom::FiatCurrencyPtr> fiat_currencies,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      false);
}

TEST_F(AssetRatioServiceUnitTest, GetCryptoCurrencies) {
  const auto url = AssetRatioService::GetCryptoCurrenciesURL(
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "");
  EXPECT_EQ(url.path(), "/service-providers/properties/crypto-currencies");
  EXPECT_EQ(url.query(),
            "includeServiceProviderDetails=false&statuses=LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&serviceProviders=BANXA%2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");

  TestGetCryptoCurrencies(
      R"([
  {
    "currencyCode": "USDT_KCC",
    "name": "#REF!",
    "chainCode": "KCC",
    "chainName": "KuCoin Community Chain",
    "chainId": "0",
    "contractAddress": "0xe41d2489571d322189246dafa5ebde1f4699f498",
    "symbolImageUrl": "https://images-currency.meld.io/crypto/USDT_KCC/symbol.png"
  },
  {
    "currencyCode": "00",
    "name": "00 Token",
    "chainCode": "ETH",
    "chainName": "Ethereum",
    "chainId": "1",
    "contractAddress": "0x111111111117dc0aa78b770fa6a738034120c302",
    "symbolImageUrl": "https://images-currency.meld.io/crypto/00/symbol.png"
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "LIVE,RECENTLY_ADDED",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoCurrencyPtr> crypto_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());            
            EXPECT_EQ(base::ranges::count_if(
                          crypto_currencies,
                          [](const auto& item) {
                            return item->currency_code == "USDT_KCC" &&
                                   item->name == "#REF!" &&
                                   item->chain_code == "KCC" &&
                                   item->chain_name == "KuCoin Community Chain" &&
                                   item->chain_id == "0" &&
                                   item->contract_address == "0xe41d2489571d322189246dafa5ebde1f4699f498" &&
                                   item->symbol_image_url ==
                                       "https://images-currency.meld.io/crypto/USDT_KCC/symbol.png";
                          }),
                      1);
            EXPECT_EQ(base::ranges::count_if(
                          crypto_currencies,
                          [](const auto& item) {
                            return item->currency_code == "00" &&
                                   item->name == "00 Token" &&
                                   item->chain_code == "ETH" &&
                                   item->chain_name == "Ethereum" &&
                                   item->chain_id == "1" &&
                                   item->contract_address == "0x111111111117dc0aa78b770fa6a738034120c302" &&
                                   item->symbol_image_url ==
                                       "https://images-currency.meld.io/crypto/00/symbol.png";
                          }),
                      1);
          }));
  TestGetCryptoCurrencies(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoCurrencyPtr> crypto_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
          }));

  TestGetCryptoCurrencies(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoCurrencyPtr> crypto_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
          }),
      true);

  TestGetCryptoCurrencies(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [&](std::vector<mojom::CryptoCurrencyPtr> crypto_currencies,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      false);
}

TEST_F(AssetRatioServiceUnitTest, GetCountries) {
  const auto url = AssetRatioService::GetCountriesURL(
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "");
  EXPECT_EQ(url.path(), "/service-providers/properties/countries");
  EXPECT_EQ(url.query(),
            "includeServiceProviderDetails=false&statuses=LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&serviceProviders=BANXA%2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");

  TestGetCountries(
      R"([
  {
    "countryCode": "AF",
    "name": "Afghanistan",
    "flagImageUrl": "https://images-country.meld.io/AF/flag.svg",
    "regions": null
  },
  {
    "countryCode": "AL",
    "name": "Albania",
    "flagImageUrl": "https://images-country.meld.io/AL/flag.svg",
    "regions": null
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "LIVE,RECENTLY_ADDED",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CountryPtr> countries,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());            
            EXPECT_EQ(base::ranges::count_if(
                          countries,
                          [](const auto& item) {
                            return item->country_code == "AF" &&
                                   item->name == "Afghanistan" &&
                                   item->flag_image_url ==
                                       "https://images-country.meld.io/AF/flag.svg";
                          }),
                      1);
            EXPECT_EQ(base::ranges::count_if(
                          countries,
                          [](const auto& item) {
                            return item->country_code == "AL" &&
                                   item->name == "Albania" &&
                                   item->flag_image_url ==
                                       "https://images-country.meld.io/AL/flag.svg";
                          }),
                      1);
          }));
  TestGetCountries(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CountryPtr> countries,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
          }));

  TestGetCountries(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CountryPtr> countries,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
          }),
      true);

  TestGetCountries(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [&](std::vector<mojom::CountryPtr> countries,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      false);

}

}  // namespace brave_wallet
