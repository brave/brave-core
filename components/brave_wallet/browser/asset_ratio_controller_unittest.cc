/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
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

void OnGetEstimatedTime(bool* callback_run,
                        bool expected_success,
                        const std::string& expected_result,
                        bool success,
                        const std::string& result) {
  EXPECT_EQ(expected_success, success);
  EXPECT_EQ(expected_result, result);
  *callback_run = true;
}

void OnGetGasOracle(
    bool* callback_run,
    brave_wallet::mojom::GasEstimation1559Ptr expected_estimation,
    brave_wallet::mojom::GasEstimation1559Ptr estimation) {
  EXPECT_EQ(expected_estimation, estimation);
  *callback_run = true;
}

}  // namespace

namespace brave_wallet {

class AssetRatioControllerUnitTest : public testing::Test {
 public:
  AssetRatioControllerUnitTest()
      : browser_context_(new content::TestBrowserContext()),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    asset_ratio_controller_.reset(
        new AssetRatioController(shared_url_loader_factory_));
  }

  ~AssetRatioControllerUnitTest() override = default;

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

 protected:
  std::unique_ptr<AssetRatioController> asset_ratio_controller_;

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(AssetRatioControllerUnitTest, GetPrice) {
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
  asset_ratio_controller_->GetPrice(
      {"bat", "link"}, {"btc", "usd"},
      brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPrice, &callback_run, true,
                     std::move(expected_prices_response)));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetPriceUppercase) {
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
  asset_ratio_controller_->GetPrice(
      {"BAT"}, {"BTC"}, brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPrice, &callback_run, true,
                     std::move(expected_prices_response)));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetPriceError) {
  std::string error = "error";
  SetErrorInterceptor(error);
  std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response;
  bool callback_run = false;
  asset_ratio_controller_->GetPrice(
      {"bat"}, {"btc"}, brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPrice, &callback_run, false,
                     std::move(expected_prices_response)));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetPriceUnexpectedResponse) {
  SetInterceptor("expecto patronum");
  std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response;
  bool callback_run = false;
  asset_ratio_controller_->GetPrice(
      {"bat"}, {"btc"}, brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPrice, &callback_run, false,
                     std::move(expected_prices_response)));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetPriceHistory) {
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
  asset_ratio_controller_->GetPriceHistory(
      "bat", brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPriceHistory, &callback_run, true,
                     std::move(expected_price_history_response)));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetPriceHistoryError) {
  std::string error = "error";
  SetErrorInterceptor(error);
  std::vector<brave_wallet::mojom::AssetTimePricePtr>
      expected_price_history_response;
  bool callback_run = false;
  asset_ratio_controller_->GetPriceHistory(
      "bat", brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPriceHistory, &callback_run, false,
                     std::move(expected_price_history_response)));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetPriceHistoryUnexpectedResponse) {
  SetInterceptor("Accio!");
  std::vector<brave_wallet::mojom::AssetTimePricePtr>
      expected_price_history_response;

  bool callback_run = false;
  asset_ratio_controller_->GetPriceHistory(
      "bat", brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&OnGetPriceHistory, &callback_run, false,
                     std::move(expected_price_history_response)));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetEstimatedTime) {
  SetInterceptor(R"(
    {
      "payload": {
        "status": "1",
        "message": "",
        "result": "3615"
      },
      "lastUpdated": "2021-09-22T21:45:40.015Z"
    }
  )");

  bool callback_run = false;
  asset_ratio_controller_->GetEstimatedTime(
      "2000000000",
      base::BindOnce(&OnGetEstimatedTime, &callback_run, true, "3615"));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetEstimatedTimeEmptyResult) {
  SetInterceptor(R"(
    {
      "payload": {
        "status": "1",
        "message": "",
        "result": ""
      },
      "lastUpdated": "2021-09-22T21:45:40.015Z"
    }
  )");

  bool callback_run = false;
  asset_ratio_controller_->GetEstimatedTime(
      "2000000000",
      base::BindOnce(&OnGetEstimatedTime, &callback_run, false, ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetEstimatedTimeServerError) {
  SetErrorInterceptor("error");
  bool callback_run = false;
  asset_ratio_controller_->GetEstimatedTime(
      "2000000000",
      base::BindOnce(&OnGetEstimatedTime, &callback_run, false, ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetGasOracle) {
  SetInterceptor(R"(
    {
      "payload": {
        "status": "1",
        "message": "",
        "result": {
          "LastBlock": "13243541",
          "SafeGasPrice": "47",
          "ProposeGasPrice": "48",
          "FastGasPrice": "49",
          "suggestBaseFee": "46.574033786",
          "gasUsedRatio": "0.27036175840958,0.0884828740801432,0.0426623303159149,0.972173412918789,0.319781207901446"
        }
      },
      "lastUpdated": "2021-09-22T21:45:40.015Z"
    }
  )");

  bool callback_run = false;
  brave_wallet::mojom::GasEstimation1559Ptr expected_estimation =
      brave_wallet::mojom::GasEstimation1559::New(
          "0x3b9aca00" /* Hex of 1 * 1e9 */,
          "0xaf16b1600" /* Hex of 47 * 1e9 */,
          "0x77359400" /* Hex of 2 * 1e9 */,
          "0xb2d05e000" /* Hex of 48 * 1e9 */,
          "0xb2d05e00" /* Hex of 3 * 1e9 */,
          "0xb68a0aa00" /* Hex of 49 * 1e9 */,
          "0xad8075b7a" /* Hex of 46574033786 */);
  asset_ratio_controller_->GetGasOracle(base::BindOnce(
      &OnGetGasOracle, &callback_run, std::move(expected_estimation)));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetGasOracleUnexpectedResponse) {
  SetInterceptor("unexpected response");
  bool callback_run = false;
  asset_ratio_controller_->GetGasOracle(
      base::BindOnce(&OnGetGasOracle, &callback_run, nullptr));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(AssetRatioControllerUnitTest, GetGasOracleServerError) {
  SetErrorInterceptor("error");
  bool callback_run = false;
  asset_ratio_controller_->GetGasOracle(
      base::BindOnce(&OnGetGasOracle, &callback_run, nullptr));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

}  // namespace brave_wallet
