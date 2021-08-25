/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
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
  ASSERT_EQ(prices[0]->from_asset, "bat");
  ASSERT_EQ(prices[0]->to_asset, "btc");
  ASSERT_EQ(prices[0]->price, "0.00001732");
  ASSERT_EQ(prices[0]->asset_timeframe_change, "8.021672460190562");

  ASSERT_EQ(prices[1]->from_asset, "bat");
  ASSERT_EQ(prices[1]->to_asset, "usd");
  ASSERT_EQ(prices[1]->price, "0.55393");
  ASSERT_EQ(prices[1]->asset_timeframe_change, "9.523443444373276");

  ASSERT_EQ(prices[2]->from_asset, "link");
  ASSERT_EQ(prices[2]->to_asset, "btc");
  ASSERT_EQ(prices[2]->price, "0.00261901");
  ASSERT_EQ(prices[2]->asset_timeframe_change, "0.5871625385632929");

  ASSERT_EQ(prices[3]->from_asset, "link");
  ASSERT_EQ(prices[3]->to_asset, "usd");
  ASSERT_EQ(prices[3]->price, "83.77");
  ASSERT_EQ(prices[3]->asset_timeframe_change, "1.7646208048244043");

  /*
  ASSERT_EQ(price, "0.694503");
  // 2 value responses happen now for the alias and the full name
  // We always parse only the first.
  json =
      R"({"payload":{"basic-attention-token":{"usd":0.694504},"bat":{"usd":0.529011}}})";
  ASSERT_TRUE(ParseAssetPrice(json, &price));
  ASSERT_EQ(price, "0.694504");
  // Invalid input
  json = R"({"payload":{"basic-attention-token": 3}})";
  ASSERT_FALSE(ParseAssetPrice(json, &price));
  json = "3";
  ASSERT_FALSE(ParseAssetPrice(json, &price));
  json = "[3]";
  ASSERT_FALSE(ParseAssetPrice(json, &price));
  json = "";
  ASSERT_FALSE(ParseAssetPrice(json, &price));
  */
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
  ASSERT_EQ(values[0]->price, "0.8201346624954003");
  base::Time date = base::Time::FromJsTime(values[0]->date.InMilliseconds());
  base::Time::Exploded exploded_time;
  date.UTCExplode(&exploded_time);
  ASSERT_EQ(exploded_time.year, 2021);
  ASSERT_EQ(exploded_time.month, 6);
  ASSERT_EQ(exploded_time.day_of_month, 3);

  ASSERT_EQ(values[1]->price, "0.8096978545029869");
  base::Time date1 = base::Time::FromJsTime(values[1]->date.InMilliseconds());
  date1.UTCExplode(&exploded_time);
  ASSERT_EQ(exploded_time.year, 2021);
  ASSERT_EQ(exploded_time.month, 6);
  ASSERT_EQ(exploded_time.day_of_month, 3);

  // Invalid input
  json = R"({"market_caps": []})";
  ASSERT_FALSE(ParseAssetPriceHistory(json, &values));
  json = "3";
  ASSERT_FALSE(ParseAssetPriceHistory(json, &values));
  json = "[3]";
  ASSERT_FALSE(ParseAssetPriceHistory(json, &values));
  json = "";
  ASSERT_FALSE(ParseAssetPriceHistory(json, &values));
}

}  // namespace brave_wallet
