/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace brave_wallet {

TEST(AssetRatioResponseParserUnitTest, ParseAssetPrices) {
  std::string json(R"([
    {
      "coin": "ETH",
      "chain_id": "0x1",
      "address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
      "price": "0.55393",
      "percentage_change_24h": "8.021672460190562",
      "vs_currency": "USD",
      "cache_status": "HIT",
      "source": "coingecko"
    },
    {
      "coin": "ETH",
      "chain_id": "0x1",
      "address": "0x514910771AF9Ca656af840dff83E8264EcF986CA",
      "price": "83.77",
      "percentage_change_24h": "0.5871625385632929",
      "vs_currency": "USD",
      "cache_status": "HIT",
      "source": "jupiter"
    }
  ])");

  std::vector<brave_wallet::mojom::AssetPricePtr> prices =
      ParseAssetPrices(ParseJson(json));
  ASSERT_EQ(prices.size(), 2UL);

  // Check BAT token (ETH chain with contract address)
  EXPECT_EQ(prices[0]->coin, mojom::CoinType::ETH);
  EXPECT_EQ(prices[0]->chain_id, "0x1");
  EXPECT_EQ(prices[0]->address, "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  EXPECT_EQ(prices[0]->price, "0.55393");
  EXPECT_EQ(prices[0]->vs_currency, "USD");
  EXPECT_EQ(prices[0]->percentage_change_24h, "8.021672460190562");
  EXPECT_EQ(prices[0]->source, mojom::AssetPriceSource::kCoingecko);

  // Check LINK token (ETH chain with contract address)
  EXPECT_EQ(prices[1]->coin, mojom::CoinType::ETH);
  EXPECT_EQ(prices[1]->chain_id, "0x1");
  EXPECT_EQ(prices[1]->address, "0x514910771AF9Ca656af840dff83E8264EcF986CA");
  EXPECT_EQ(prices[1]->price, "83.77");
  EXPECT_EQ(prices[1]->vs_currency, "USD");
  EXPECT_EQ(prices[1]->percentage_change_24h, "0.5871625385632929");
  EXPECT_EQ(prices[1]->source, mojom::AssetPriceSource::kJupiter);

  // Test with native tokens
  prices.clear();
  json = R"([
    {
      "coin": "BTC",
      "chain_id": "bitcoin_mainnet",
      "price": "45000.0",
      "percentage_change_24h": "2.5",
      "vs_currency": "USD",
      "cache_status": "HIT",
      "source": "coingecko"
    },
    {
      "coin": "ETH",
      "chain_id": "0x1",
      "price": "2800.0",
      "percentage_change_24h": "-1.2",
      "vs_currency": "USD",
      "cache_status": "HIT",
      "source": "coingecko"
    }
  ])";
  prices = ParseAssetPrices(ParseJson(json));
  ASSERT_EQ(prices.size(), 2UL);

  // Check BTC native token
  EXPECT_EQ(prices[0]->coin, mojom::CoinType::BTC);
  EXPECT_EQ(prices[0]->price, "45000.0");
  EXPECT_EQ(prices[0]->vs_currency, "USD");
  EXPECT_EQ(prices[0]->percentage_change_24h, "2.5");
  EXPECT_EQ(prices[0]->source, mojom::AssetPriceSource::kCoingecko);

  // Check ETH native token
  EXPECT_EQ(prices[1]->coin, mojom::CoinType::ETH);
  EXPECT_EQ(prices[1]->chain_id, "0x1");
  EXPECT_EQ(prices[1]->price, "2800.0");
  EXPECT_EQ(prices[1]->vs_currency, "USD");
  EXPECT_EQ(prices[1]->percentage_change_24h, "-1.2");
  EXPECT_EQ(prices[1]->source, mojom::AssetPriceSource::kCoingecko);

  // Test with EUR currency
  prices.clear();
  json = R"([
    {
      "coin": "BTC",
      "chain_id": "bitcoin_mainnet",
      "price": "42000.0",
      "percentage_change_24h": "0.1",
      "vs_currency": "EUR",
      "cache_status": "HIT",
      "source": "coingecko"
    }
  ])";
  prices = ParseAssetPrices(ParseJson(json));
  ASSERT_EQ(prices.size(), 1UL);
  EXPECT_EQ(prices[0]->coin, mojom::CoinType::BTC);
  EXPECT_EQ(prices[0]->price, "42000.0");
  EXPECT_EQ(prices[0]->vs_currency, "EUR");
  EXPECT_EQ(prices[0]->percentage_change_24h, "0.1");
  EXPECT_EQ(prices[0]->source, mojom::AssetPriceSource::kCoingecko);

  // Test with empty percentage change 24h
  prices.clear();
  json = R"([
    {
      "coin": "BTC",
      "chain_id": "bitcoin_mainnet",
      "price": "42000.0",
      "percentage_change_24h": "",
      "vs_currency": "EUR",
      "cache_status": "HIT",
      "source": "coingecko"
    }
  ])";
  prices = ParseAssetPrices(ParseJson(json));
  ASSERT_EQ(prices.size(), 1UL);
  EXPECT_EQ(prices[0]->coin, mojom::CoinType::BTC);
  EXPECT_EQ(prices[0]->price, "42000.0");
  EXPECT_EQ(prices[0]->vs_currency, "EUR");
  EXPECT_EQ(prices[0]->percentage_change_24h, "");
  EXPECT_EQ(prices[0]->source, mojom::AssetPriceSource::kCoingecko);

  // Test with unknown source
  prices.clear();
  json = R"([
    {
      "coin": "BTC",
      "chain_id": "bitcoin_mainnet",
      "price": "42000.0",
      "percentage_change_24h": "0.1",
      "vs_currency": "EUR",
      "cache_status": "HIT",
      "source": "unknown"
    }
  ])";
  prices = ParseAssetPrices(ParseJson(json));
  ASSERT_EQ(prices.size(), 1UL);
  EXPECT_EQ(prices[0]->source, mojom::AssetPriceSource::kUnknown);

  // Invalid json input
  prices = ParseAssetPrices(ParseJson("{\"result\": \"not an array\"}"));
  EXPECT_EQ(prices.size(),
            0UL);  // Should return empty vector for invalid input

  prices = ParseAssetPrices(ParseJson("3615"));
  EXPECT_EQ(prices.size(), 0UL);

  prices = ParseAssetPrices(base::Value());
  EXPECT_EQ(prices.size(), 0UL);

  // Empty response array
  prices = ParseAssetPrices(ParseJson("[]"));
  EXPECT_EQ(prices.size(), 0UL);

  // Response missing required fields
  json = R"([{"coin": "BTC"}])";  // missing price
  prices = ParseAssetPrices(ParseJson(json));
  EXPECT_EQ(prices.size(), 0UL);  // Should skip invalid entries
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
  ASSERT_TRUE(ParseAssetPriceHistory(ParseJson(json), &values));
  ASSERT_EQ(values.size(), 2UL);
  EXPECT_EQ(values[0]->price, "0.8201346624954003");
  base::Time date = base::Time::FromMillisecondsSinceUnixEpoch(
      values[0]->date.InMilliseconds());
  base::Time::Exploded exploded_time;
  date.UTCExplode(&exploded_time);
  EXPECT_EQ(exploded_time.year, 2021);
  EXPECT_EQ(exploded_time.month, 6);
  EXPECT_EQ(exploded_time.day_of_month, 3);

  EXPECT_EQ(values[1]->price, "0.8096978545029869");
  base::Time date1 = base::Time::FromMillisecondsSinceUnixEpoch(
      values[1]->date.InMilliseconds());
  date1.UTCExplode(&exploded_time);
  EXPECT_EQ(exploded_time.year, 2021);
  EXPECT_EQ(exploded_time.month, 6);
  EXPECT_EQ(exploded_time.day_of_month, 3);

  // Invalid input
  json = R"({"market_caps": []})";
  EXPECT_FALSE(ParseAssetPriceHistory(ParseJson(json), &values));
  json = "3";
  EXPECT_FALSE(ParseAssetPriceHistory(ParseJson(json), &values));
  json = "[3]";
  EXPECT_FALSE(ParseAssetPriceHistory(ParseJson(json), &values));

  EXPECT_FALSE(ParseAssetPriceHistory(base::Value(), &values));
}

TEST(AssetRatioResponseParserUnitTest, ParseCoinMarkets) {
  // https://ratios.rewards.brave.software/v2/market/provider/coingecko\?vsCurrency\=usd\&limit\=2
  std::string json(R"(
    {
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
    }
  )");

  auto parsed_values = ParseCoinMarkets(ParseJson(json));
  ASSERT_TRUE(parsed_values);

  std::vector<brave_wallet::mojom::CoinMarketPtr>& values = *parsed_values;
  ASSERT_EQ(values.size(), 1UL);
  EXPECT_EQ(values[0]->id, "bitcoin");
  EXPECT_EQ(values[0]->symbol, "btc");
  EXPECT_EQ(values[0]->name, "Bitcoin");
  EXPECT_EQ(values[0]->image,
            "https://assets.coingecko.com/coins/images/1/large/"
            "bitcoin.png?1547033579");
  EXPECT_EQ(values[0]->market_cap, 727960800075);
  EXPECT_EQ(values[0]->market_cap_rank, uint32_t(1));
  EXPECT_EQ(values[0]->current_price, 38357);
  EXPECT_EQ(values[0]->price_change_24h, -1229.64683216549);
  EXPECT_EQ(values[0]->price_change_percentage_24h, -3.10625);
  EXPECT_EQ(values[0]->total_volume, 17160995925);

  // Invalid input
  json = R"({"id": []})";
  EXPECT_FALSE(ParseCoinMarkets(ParseJson(json)));
  json = "3";
  EXPECT_FALSE(ParseCoinMarkets(ParseJson(json)));
  json = "[3]";
  EXPECT_FALSE(ParseCoinMarkets(ParseJson(json)));
}

TEST(AssetRatioResponseParserUnitTest, ParseStripeBuyURL) {
  std::string json(R"({
      "url": "https://crypto.link.com?session_hash=abcdefgh"
  })");

  auto parsed_value = ParseStripeBuyURL(ParseJson(json));
  ASSERT_TRUE(parsed_value);
  EXPECT_EQ(*parsed_value, "https://crypto.link.com?session_hash=abcdefgh");

  // Invalid input
  json = R"({"url": []})";
  EXPECT_FALSE(ParseStripeBuyURL(ParseJson(json)));
  json = "3";
  EXPECT_FALSE(ParseStripeBuyURL(ParseJson(json)));
  json = "[3]";
  EXPECT_FALSE(ParseStripeBuyURL(ParseJson(json)));
}

}  // namespace brave_wallet
