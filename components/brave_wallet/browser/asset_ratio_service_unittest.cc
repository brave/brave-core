/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
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

}  // namespace

namespace brave_wallet {

class AssetRatioServiceUnitTest : public testing::Test {
 public:
  AssetRatioServiceUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    asset_ratio_service_.reset(
        new AssetRatioService(shared_url_loader_factory_));
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

  void GetTokenInfo(const std::string& contract_address,
                    mojom::BlockchainTokenPtr expected_token) {
    base::RunLoop run_loop;
    asset_ratio_service_->GetTokenInfo(
        contract_address,
        base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
          EXPECT_EQ(token, expected_token);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetBuyUrlV1(mojom::OnRampProvider on_ramp_provider,
                       const std::string& chain_id,
                       const std::string& address,
                       const std::string& symbol,
                       const std::string& amount,
                       const std::string& currency_code,
                       const std::string& expected_url,
                       absl::optional<std::string> expected_error) {
    base::RunLoop run_loop;
    asset_ratio_service_->GetBuyUrlV1(
        on_ramp_provider, chain_id, address, symbol, amount, currency_code,
        base::BindLambdaForTesting(
            [&](const std::string& url,
                const absl::optional<std::string>& error) {
              EXPECT_EQ(url, expected_url);
              EXPECT_EQ(error, expected_error);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

 protected:
  std::unique_ptr<AssetRatioService> asset_ratio_service_;

 private:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(AssetRatioServiceUnitTest, GetBuyUrlV1Wyre) {
  TestGetBuyUrlV1(
      mojom::OnRampProvider::kWyre, mojom::kMainnetChainId, "0xdeadbeef",
      "USDC", "99.99", "USD",
      "https://pay.sendwyre.com/"
      "?dest=ethereum%3A0xdeadbeef&sourceCurrency=USD&destCurrency=USDC&amount="
      "99.99&accountId=AC_MGNVBGHPA9T&paymentMethod=debit-card",
      absl::nullopt);
}

TEST_F(AssetRatioServiceUnitTest, GetBuyUrlV1Ramp) {
  TestGetBuyUrlV1(mojom::OnRampProvider::kRamp, mojom::kMainnetChainId,
                  "0xdeadbeef", "USDC", "55000000", "USD",
                  "https://buy.ramp.network/"
                  "?userAddress=0xdeadbeef&swapAsset=USDC&fiatValue=55000000"
                  "&fiatCurrency=USD&hostApiKey="
                  "8yxja8782as5essk2myz3bmh4az6gpq4nte9n2gf",
                  absl::nullopt);
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
                  "a537-4f5d-ab4d-9916739560b1",
                  absl::nullopt);

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

  base::RunLoop().RunUntilIdle();
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

  base::RunLoop().RunUntilIdle();
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

  base::RunLoop().RunUntilIdle();
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

  base::RunLoop().RunUntilIdle();
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

  base::RunLoop().RunUntilIdle();
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
  base::RunLoop().RunUntilIdle();
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

  base::RunLoop().RunUntilIdle();
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

TEST_F(AssetRatioServiceUnitTest, GetTokenInfoURL) {
  std::string url(kAssetRatioBaseURL);
  EXPECT_EQ(url +
                "v2/etherscan/"
                "passthrough?module=token&action=tokeninfo&contractaddress="
                "0xdac17f958d2ee523a2206206994597c13d831ec7",
            AssetRatioService::GetTokenInfoURL(
                "0xdac17f958d2ee523a2206206994597c13d831ec7")
                .spec());
}

TEST_F(AssetRatioServiceUnitTest, GetTokenInfo) {
  SetInterceptor(R"(
    {
      "payload": {
        "status": "1",
        "message": "OK",
        "result": [{
          "contractAddress": "0xdac17f958d2ee523a2206206994597c13d831ec7",
          "tokenName": "Tether USD",
          "symbol": "USDT",
          "divisor": "6",
          "tokenType": "ERC20",
          "totalSupply": "39828710009874796",
          "blueCheckmark": "true",
          "description": "Tether gives you the joint benefits of open...",
          "website": "https://tether.to/",
          "email": "support@tether.to",
          "blog": "https://tether.to/category/announcements/",
          "reddit": "",
          "slack": "",
          "facebook": "",
          "twitter": "https://twitter.com/Tether_to",
          "bitcointalk": "",
          "github": "",
          "telegram": "",
          "wechat": "",
          "linkedin": "",
          "discord": "",
          "whitepaper": "https://path/to/TetherWhitePaper.pdf",
          "tokenPriceUSD": "1.000000000000000000"
        }]
      },
      "lastUpdated": "2021-12-09T22:02:23.187Z"
    }
  )");
  GetTokenInfo(
      "0xdac17f958d2ee523a2206206994597c13d831ec7",
      mojom::BlockchainToken::New("0xdAC17F958D2ee523a2206206994597C13D831ec7",
                                  "Tether USD", "", true, false, "USDT", 6,
                                  true, "", "", "0x1", mojom::CoinType::ETH));

  SetInterceptor("unexpected response");
  GetTokenInfo("0xdac17f958d2ee523a2206206994597c13d831ec7", nullptr);

  SetErrorInterceptor("error");
  GetTokenInfo("0xdac17f958d2ee523a2206206994597c13d831ec7", nullptr);
}

}  // namespace brave_wallet
