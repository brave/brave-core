/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/crypto_dot_com/browser/crypto_dot_com_json_parser.h"

#include <map>
#include <vector>

#include "brave/components/crypto_dot_com/browser/crypto_dot_com_service.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"

// npm run test -- brave_unit_tests --filter=CryptoDotComJSONParserTest.*

namespace {

double GetValueFromStringMap(const std::map<std::string, double>& map,
                             const std::string& key) {
  double value = -1.0;
  std::map<std::string, double>::const_iterator it = map.find(key);
  if (it != map.end()) {
    value = it->second;
  }
  return value;
}

std::string GetValueFromStringMap(
    const std::map<std::string, std::string>& map,
    const std::string& key) {
  std::string value;
  std::map<std::string, std::string>::const_iterator it =
      map.find(key);
  if (it != map.end()) {
    value = it->second;
  }
  return value;
}

std::vector<std::map<std::string, std::string>> GetVectorFromStringRecordMap(
    const std::map<std::string, std::vector<
    std::map<std::string, std::string>>>& map,
    const std::string& key) {
  std::vector<std::map<std::string, std::string>> value;
  std::map<std::string, std::vector<
      std::map<std::string, std::string>>>::const_iterator it =
      map.find(key);
  if (it != map.end()) {
    value = it->second;
  }
  return value;
}

typedef testing::Test CryptoDotComJSONParserTest;

TEST_F(CryptoDotComJSONParserTest, GetTickerInfoFromJSON) {
  CryptoDotComTickerInfo info;
  ASSERT_TRUE(CryptoDotComJSONParser::GetTickerInfoFromJSON(R"(
      {
        "response": {
            "code": 0,
            "method": "public/get-ticker",
            "result": {
                "instrument_name": "BTC_USDT",
                "data": {
                    "i": "BTC_USDT",
                    "b": 11760.03,
                    "k": 11762.97,
                    "a": 11759.2,
                    "t": 1598254503038,
                    "v": 786.863035,
                    "h": 11773.98,
                    "l": 11520.55,
                    "c": 148.95
                }
            }
        }
      })", &info));
  double info_price = GetValueFromStringMap(info, "price");
  double info_volume = GetValueFromStringMap(info, "volume");
  constexpr double kTargetPrice = 11759.2;
  constexpr double kTargetVolume = 9164802.2873492744;
  EXPECT_EQ(kTargetPrice, info_price);
  EXPECT_EQ(kTargetVolume, info_volume);
}

TEST_F(CryptoDotComJSONParserTest, GetChartDataFromJSON) {
  CryptoDotComChartData data;
  ASSERT_TRUE(CryptoDotComJSONParser::GetChartDataFromJSON(R"(
      {
        "response": {
            "code": 0,
            "method": "public/get-candlestick",
            "result": {
                "instrument_name": "BTC_USDT",
                "depth": 1,
                "interval": "1D",
                "data": [
                    {
                        "t": 1598227200000,
                        "o": 11646.9,
                        "h": 11792.51,
                        "l": 11594.55,
                        "c": 11787.25,
                        "v": 228.290252
                    },
                    {
                        "t": 16982337200000,
                        "o": 12646.9,
                        "h": 13882.51,
                        "l": 14734.55,
                        "c": 15787.25,
                        "v": 268.290252
                    }
                ]
            }
        }
      })", &data));

  std::map<std::string, double> first_point = data.front();
  std::map<std::string, double> last_point = data.back();

  double t_f = GetValueFromStringMap(first_point, "t");
  double o_f = GetValueFromStringMap(first_point, "o");
  double h_f = GetValueFromStringMap(first_point, "h");
  double l_f = GetValueFromStringMap(first_point, "l");
  double c_f = GetValueFromStringMap(first_point, "c");
  double v_f = GetValueFromStringMap(first_point, "v");

  ASSERT_EQ(t_f, 1598227200000.000000);
  ASSERT_EQ(o_f, 11646.900000);
  ASSERT_EQ(h_f, 11792.510000);
  ASSERT_EQ(l_f, 11594.550000);
  ASSERT_EQ(c_f, 11787.250000);
  ASSERT_EQ(v_f, 228.290252);

  double t_l = GetValueFromStringMap(last_point, "t");
  double o_l = GetValueFromStringMap(last_point, "o");
  double h_l = GetValueFromStringMap(last_point, "h");
  double l_l = GetValueFromStringMap(last_point, "l");
  double c_l = GetValueFromStringMap(last_point, "c");
  double v_l = GetValueFromStringMap(last_point, "v");

  ASSERT_EQ(t_l, 16982337200000.000000);
  ASSERT_EQ(o_l, 12646.900000);
  ASSERT_EQ(h_l, 13882.510000);
  ASSERT_EQ(l_l, 14734.550000);
  ASSERT_EQ(c_l, 15787.250000);
  ASSERT_EQ(v_l, 268.290252);
}

TEST_F(CryptoDotComJSONParserTest, GetPairsFromJSON) {
  CryptoDotComSupportedPairs pairs;
  ASSERT_TRUE(CryptoDotComJSONParser::GetPairsFromJSON(R"(
      {
        "response": {
            "code": 0,
            "method": "public/get-instruments",
            "result": {
                "instruments": [
                    {
                        "instrument_name": "NEO_BTC",
                        "quote_currency": "BTC",
                        "base_currency": "NEO",
                        "price_decimals": 6,
                        "quantity_decimals": 3
                    },
                    {
                        "instrument_name": "ETH_BTC",
                        "quote_currency": "BTC",
                        "base_currency": "ETH",
                        "price_decimals": 6,
                        "quantity_decimals": 3
                    }
                ]
            }
        }
      })", &pairs));

  std::map<std::string, std::string> first_pair = pairs.front();
  std::map<std::string, std::string> last_pair = pairs.back();

  std::string pair_f = GetValueFromStringMap(first_pair, "pair");
  std::string quote_f = GetValueFromStringMap(first_pair, "quote");
  std::string base_f = GetValueFromStringMap(first_pair, "base");

  ASSERT_EQ(pair_f, "NEO_BTC");
  ASSERT_EQ(quote_f, "BTC");
  ASSERT_EQ(base_f, "NEO");

  std::string pair_l = GetValueFromStringMap(last_pair, "pair");
  std::string quote_l = GetValueFromStringMap(last_pair, "quote");
  std::string base_l = GetValueFromStringMap(last_pair, "base");

  ASSERT_EQ(pair_l, "ETH_BTC");
  ASSERT_EQ(quote_l, "BTC");
  ASSERT_EQ(base_l, "ETH");
}

TEST_F(CryptoDotComJSONParserTest, GetRankingsFromJSON) {
  CryptoDotComAssetRankings rankings;
  ASSERT_TRUE(CryptoDotComJSONParser::GetRankingsFromJSON(R"(
      {
        "response": {
            "code": 0,
            "result": {
                "gainers": [
                    {
                        "currency": "BTC",
                        "currency_name": "Bitcoin",
                        "instrument_name": "BTC_USDT",
                        "image_url": "",
                        "last_price": "10000.00",
                        "percent_change": "50.11"
                    },
                    {
                        "currency": "XRP",
                        "currency_name": "XRP",
                        "instrument_name": "XRP_USDT",
                        "image_url": "",
                        "last_price": "0.10",
                        "percent_change": "-20.12"
                    }
                ]
            }
        }
      })", &rankings));

  std::vector<std::map<std::string, std::string>> gainers =
      GetVectorFromStringRecordMap(rankings, "gainers");
  std::vector<std::map<std::string, std::string>> losers =
      GetVectorFromStringRecordMap(rankings, "losers");

  std::map<std::string, std::string> gainer = gainers.front();
  std::map<std::string, std::string> loser = losers.front();

  std::string pair_g = GetValueFromStringMap(gainer, "pair");
  std::string change_g = GetValueFromStringMap(gainer, "percentChange");
  std::string price_g = GetValueFromStringMap(gainer, "lastPrice");

  ASSERT_EQ(pair_g, "BTC_USDT");
  ASSERT_EQ(change_g, "50.11");
  ASSERT_EQ(price_g, "10000.00");

  std::string pair_l = GetValueFromStringMap(loser, "pair");
  std::string change_l = GetValueFromStringMap(loser, "percentChange");
  std::string price_l = GetValueFromStringMap(loser, "lastPrice");

  ASSERT_EQ(pair_l, "XRP_USDT");
  ASSERT_EQ(change_l, "-20.12");
  ASSERT_EQ(price_l, "0.10");
}

TEST_F(CryptoDotComJSONParserTest, GetAccountBalancesFromJSON) {
  base::Value valid = CryptoDotComJSONParser::GetValidAccountBalances(R"(
      {
        "code": "0",
        "result": {
          "total_balance":"100000.33",
          "accounts":[
            {
              "stake":"0",
              "balance":"0",
              "available":"0",
              "currency":"BAT",
              "currency_decimals":8,
              "order":"0"
            },
            {
              "stake":"0",
              "balance":"0",
              "available":"0",
              "currency":"ETH",
              "currency_decimals":8,
              "order":"0"
            }
          ]
        }
      })");
  EXPECT_TRUE(!valid.is_none());
  const base::Value* accounts = valid.FindListKey("accounts");
  // have 2 valid currency balances.
  EXPECT_EQ(2UL, accounts->GetList().size());

  base::Value valid_2 = CryptoDotComJSONParser::GetValidAccountBalances(R"(
      {
        "code": "0",
        "result": {
          "total_balance":"100000.33",
          "accounts":[
            {
              "stake":"0",
              "balance":"0",
              "available":"0",
              "currency":"BAT",
              "currency_decimals":8,
              "order":"0"
            },
            {
              "stake":"0",
              "balance":"0",
              "available":"0",
              "currency":"ETH",
              "order":"0"
            }
          ]
        }
      })");
  EXPECT_TRUE(!valid_2.is_none());
  const base::Value* accounts_2 = valid_2.FindListKey("accounts");
  // have 1 valid currency balances because sconed balance doesn't have
  // 'currency_decimal' property.
  EXPECT_EQ(1UL, accounts_2->GetList().size());

  // All included balances are invalid - doesn't have 'currency_decimal' props.
  base::Value invalid_1 = CryptoDotComJSONParser::GetValidAccountBalances(R"(
      {
        "code": "0",
        "result": {
          "total_balance":"100000.33",
          "accounts":[
            {
              "stake":"0",
              "balance":"0",
              "available":"0",
              "currency":"BAT",
              "order":"0"
            },
            {
              "stake":"0",
              "balance":"0",
              "available":"0",
              "currency":"ETH",
              "order":"0"
            }
          ]
        }
      })");
  EXPECT_TRUE(invalid_1.is_none());

  base::Value invalid_2 = CryptoDotComJSONParser::GetValidAccountBalances(R"(
      {
        "code": "1",
        "result": {
        }
      })");
  EXPECT_TRUE(invalid_2.is_none());
}

TEST_F(CryptoDotComJSONParserTest, GetNewsEventsFromJSON) {
  base::Value valid = CryptoDotComJSONParser::GetValidNewsEvents(R"(
      {
        "code": "0",
        "result":{
          "events":[
            {
              "layout":"announcement",
              "updated_at":"2020-11-03T07:17:56.891Z",
              "redirect_title":"More here",
              "redirect_type":"url",
              "content":"November Updates",
              "redirect_url":"https://blog.crypto.com/crypto-com-november-2019-updates/"
            }
          ]
        }
      })");
  EXPECT_TRUE(valid.is_list());
  EXPECT_EQ(1UL, valid.GetList().size());

  base::Value invalid = CryptoDotComJSONParser::GetValidNewsEvents(R"(
      {
        "code": "0",
        "result":{
          "events":[
            {
              "layout":"announcement",
              "redirect_title":"More here",
              "redirect_type":"url",
              "content":"November Updates",
              "redirect_url":"https://blog.crypto.com/crypto-com-november-2019-updates/"
            }
          ]
        }
      })");
  // event doesn't have 'updated_at' props.
  EXPECT_TRUE(invalid.is_none());
}

}  // namespace
