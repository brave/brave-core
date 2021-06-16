/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(AssetRatioResponseParserUnitTest, ParseAssetPrice) {
  std::string json(R"({"basic-attention-token":{"usd":0.694503}})");
  std::string price;
  ASSERT_TRUE(ParseAssetPrice(json, &price));
  ASSERT_EQ(price, "0.694503");

  // Invalid input
  json = R"({"basic-attention-token": 3})";
  ASSERT_FALSE(ParseAssetPrice(json, &price));
  json = "3";
  ASSERT_FALSE(ParseAssetPrice(json, &price));
  json = "[3]";
  ASSERT_FALSE(ParseAssetPrice(json, &price));
  json = "";
  ASSERT_FALSE(ParseAssetPrice(json, &price));
}

TEST(AssetRatioResponseParserUnitTest, ParseAssetPriceHistory) {
  // https://ratios.bsg.bravesoftware.com/v2/history/coingecko/basic-attention-token/usd/2021-06-03T15%3A00%3A00.000Z/2021-06-03T18%3A00%3A00.000Z
  std::string json(R"(
    {
      "prices":[[1622733088498,0.8201346624954003],[1622737203757,0.8096978545029869]],
      "market_caps":[[1622733088498,1223507820.383275],[1622737203757,1210972881.4928021]],
      "total_volumes":[[1622733088498,163426828.00299588],[1622737203757,157618689.0971025]]
    } 
  )");

  std::vector<AssetTimePrice> values;
  ASSERT_TRUE(ParseAssetPriceHistory(json, &values));
  ASSERT_EQ(values.size(), 2UL);
  ASSERT_EQ(values[0].second, "0.8201346624954003");
  auto date = base::UTF16ToUTF8(TimeFormatShortDateAndTime(values[0].first));
  ASSERT_EQ(date, "6/3/21, 11:11:28 AM");

  ASSERT_EQ(values[1].second, "0.8096978545029869");
  date = base::UTF16ToUTF8(TimeFormatShortDateAndTime(values[1].first));
  ASSERT_EQ(date, "6/3/21, 12:20:03 PM");

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
