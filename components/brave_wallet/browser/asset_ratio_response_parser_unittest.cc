/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(AssetRatioResponseParserUnitTest, ParseAssetPrice) {
  std::string json(R"({
     "payload":{
       "basic-attention-token":{
         "btc":0.00001732,
         "btc_timeframe_change":8.021672460190562,
         "usd":0.55393,
         "usd_timeframe_change":9.523443444373276
       },
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

  std::vector<brave_wallet::mojom::AssetPricePtr> prices;
  ASSERT_TRUE(ParseAssetPrice(json, {"bat", "link"}, {"btc", "usd"}, &prices));
  ASSERT_EQ(prices.size(), 4UL);
  EXPECT_EQ(prices[0]->from_asset, "bat");
  EXPECT_EQ(prices[0]->to_asset, "btc");
  EXPECT_EQ(prices[0]->price, "0.00001732");
  EXPECT_EQ(prices[0]->asset_timeframe_change, "8.021672460190562");

  EXPECT_EQ(prices[1]->from_asset, "bat");
  EXPECT_EQ(prices[1]->to_asset, "usd");
  EXPECT_EQ(prices[1]->price, "0.55393");
  EXPECT_EQ(prices[1]->asset_timeframe_change, "9.523443444373276");

  EXPECT_EQ(prices[2]->from_asset, "link");
  EXPECT_EQ(prices[2]->to_asset, "btc");
  EXPECT_EQ(prices[2]->price, "0.00261901");
  EXPECT_EQ(prices[2]->asset_timeframe_change, "0.5871625385632929");

  EXPECT_EQ(prices[3]->from_asset, "link");
  EXPECT_EQ(prices[3]->to_asset, "usd");
  EXPECT_EQ(prices[3]->price, "83.77");
  EXPECT_EQ(prices[3]->asset_timeframe_change, "1.7646208048244043");

  // Unexpected json for inputs
  EXPECT_FALSE(
      ParseAssetPrice(json, {"A1", "A2", "A3"}, {"B1", "B2", "B3"}, &prices));
  EXPECT_FALSE(ParseAssetPrice(json, {"A1"}, {"B1", "B2"}, &prices));
  EXPECT_FALSE(ParseAssetPrice(json, {"A1", "A2"}, {"B1"}, &prices));
}

TEST(AssetRatioResponseParserUnitTest, ParseAssetPriceHistory) {
  // https://ratios.bsg.bravesoftware.com/v2/history/coingecko/basic-attention-token/usd/2021-06-03T15%3A00%3A00.000Z/2021-06-03T18%3A00%3A00.000Z
  std::string json(R"(
    {
      "payload": {
        "prices":[[1622733088498,0.8201346624954003],[1622737203757,0.8096978545029869]],
        "market_caps":[[1622733088498,1223507820.383275],[1622737203757,1210972881.4928021]],
        "total_volumes":[[1622733088498,163426828.00299588],[1622737203757,157618689.0971025]]
      }
    }
  )");

  std::vector<brave_wallet::mojom::AssetTimePricePtr> values;
  ASSERT_TRUE(ParseAssetPriceHistory(json, &values));
  ASSERT_EQ(values.size(), 2UL);
  EXPECT_EQ(values[0]->price, "0.8201346624954003");
  base::Time date = base::Time::FromJsTime(values[0]->date.InMilliseconds());
  base::Time::Exploded exploded_time;
  date.UTCExplode(&exploded_time);
  EXPECT_EQ(exploded_time.year, 2021);
  EXPECT_EQ(exploded_time.month, 6);
  EXPECT_EQ(exploded_time.day_of_month, 3);

  EXPECT_EQ(values[1]->price, "0.8096978545029869");
  base::Time date1 = base::Time::FromJsTime(values[1]->date.InMilliseconds());
  date1.UTCExplode(&exploded_time);
  EXPECT_EQ(exploded_time.year, 2021);
  EXPECT_EQ(exploded_time.month, 6);
  EXPECT_EQ(exploded_time.day_of_month, 3);

  // Invalid input
  json = R"({"market_caps": []})";
  EXPECT_FALSE(ParseAssetPriceHistory(json, &values));
  json = "3";
  EXPECT_FALSE(ParseAssetPriceHistory(json, &values));
  json = "[3]";
  EXPECT_FALSE(ParseAssetPriceHistory(json, &values));
  json = "";
  EXPECT_FALSE(ParseAssetPriceHistory(json, &values));
}

TEST(AssetRatioResponseParserUnitTest, ParseEstimatedTime) {
  std::string json(R"(
    {
      "payload": {
        "status": "1",
        "message": "",
        "result": "3615"
      },
      "lastUpdated": "2021-09-22T21:45:40.015Z"
    }
  )");

  EXPECT_EQ(ParseEstimatedTime(json), "3615");

  // Invalid json input
  EXPECT_EQ(ParseEstimatedTime("{\"result\": \"3615\"}"), "");
  EXPECT_EQ(ParseEstimatedTime("3615"), "");
  EXPECT_EQ(ParseEstimatedTime("[3615]"), "");
  EXPECT_EQ(ParseEstimatedTime(""), "");
}

TEST(AssetRatioResponseParserUnitTest, ParseGasOracle) {
  std::string json(R"(
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

  brave_wallet::mojom::GasEstimation1559Ptr expected_estimation =
      brave_wallet::mojom::GasEstimation1559::New(
          "0x3b9aca00" /* Hex of 1 * 1e9 */,
          "0xaf16b1600" /* Hex of 47 * 1e9 */,
          "0x77359400" /* Hex of 2 * 1e9 */,
          "0xb2d05e000" /* Hex of 48 * 1e9 */,
          "0xb2d05e00" /* Hex of 3 * 1e9 */,
          "0xb68a0aa00" /* Hex of 49 * 1e9 */,
          "0xad8075b7a" /* Hex of 46574033786 */);
  EXPECT_EQ(ParseGasOracle(json), expected_estimation);

  // Test suggest base fee with more than 9 digis in the fraction part.
  base::ReplaceFirstSubstringAfterOffset(&json, 0, "46.574033786",
                                         "46.5740337861");
  EXPECT_EQ(ParseGasOracle(json), expected_estimation);

  base::ReplaceFirstSubstringAfterOffset(&json, 0, "46.5740337861", "46.57403");
  expected_estimation->base_fee_per_gas = "0xad8074cb0";  // 46.57403 * 1e9
  EXPECT_EQ(ParseGasOracle(json), expected_estimation);

  // Test suggest base fee without point.
  base::ReplaceFirstSubstringAfterOffset(&json, 0, "46.57403", "46");
  expected_estimation->base_fee_per_gas = "0xab5d04c00";  // 46 * 1e9
  EXPECT_EQ(ParseGasOracle(json), expected_estimation);

  const std::string valid_json = json;
  // Test if safe_gas_price is not integer.
  base::ReplaceFirstSubstringAfterOffset(&json, 0, "47", "47.2");
  EXPECT_FALSE(ParseGasOracle(json));

  // Test if propose_gas_price is not integer.
  json = valid_json;
  base::ReplaceFirstSubstringAfterOffset(&json, 0, "48", "48.3");
  EXPECT_FALSE(ParseGasOracle(json));

  // Test if fast_gas_price is not integer.
  json = valid_json;
  base::ReplaceFirstSubstringAfterOffset(&json, 0, "49", "49.3");
  EXPECT_FALSE(ParseGasOracle(json));

  // Test invalid json.
  json = R"(
    {
      "payload": {
        "status": "1",
        "message": "",
        "result": {
          "LastBlock": "13243541",
          "SafeGasPrice": "47",
          "ProposeGasPrice": "48",
          "FastGasPrice": "49",
          "gasUsedRatio": "0.27036175840958,0.0884828740801432,0.0426623303159149,0.972173412918789,0.319781207901446"
        }
      },
      "lastUpdated": "2021-09-22T21:45:40.015Z"
    }
  )";
  EXPECT_FALSE(ParseGasOracle(json));  // Missing required base fee field.
  EXPECT_FALSE(ParseGasOracle("{\"result\": \"3615\"}"));
  EXPECT_FALSE(ParseGasOracle(""));
}

}  // namespace brave_wallet
