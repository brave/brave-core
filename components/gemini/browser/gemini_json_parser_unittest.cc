/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/gemini/browser/gemini_json_parser.h"

#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"

// npm run test -- brave_unit_tests --filter=GeminiJSONParserTest.*

namespace {

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

typedef testing::Test GeminiJSONParserTest;

TEST_F(GeminiJSONParserTest, GetAccountBalancesFromJSON) {
  std::map<std::string, std::string> balances;
  ASSERT_TRUE(GeminiJSONParser::GetAccountBalancesFromJSON(R"(
      {
        "data": [
          {
              "type": "exchange",
              "currency": "BTC",
              "amount": "1154.62034001",
              "available": "1129.10517279",
              "availableForWithdrawal": "1129.10517279"
          },
          {
              "type": "exchange",
              "currency": "USD",
              "amount": "18722.79",
              "available": "14481.62",
              "availableForWithdrawal": "14481.62"
          },
          {
              "type": "exchange",
              "currency": "ETH",
              "amount": "20124.50369697",
              "available": "20124.50369697",
              "availableForWithdrawal": "20124.50369697"
          }
        ]
      })", &balances));

  std::string usd_balance = GetValueFromStringMap(balances, "USD");
  std::string btc_balance = GetValueFromStringMap(balances, "BTC");
  std::string eth_balance = GetValueFromStringMap(balances, "ETH");

  ASSERT_EQ(usd_balance, "14481.62");
  ASSERT_EQ(btc_balance, "1129.10517279");
  ASSERT_EQ(eth_balance, "20124.50369697");
}

TEST_F(GeminiJSONParserTest, GetTokensFromJSON) {
  std::string access_token;
  std::string refresh_token;

  ASSERT_TRUE(GeminiJSONParser::GetTokensFromJSON(R"(
      {
        "access_token": "access-XXX-XXX-XXX-XXX",
        "refresh_token": "refresh-XXX-XXX-XXX-XXX",
        "scope": "Trader",
        "token_type": "Bearer",
        "expires_in": 60000
      })", &access_token, &refresh_token));

  ASSERT_EQ(access_token, "access-XXX-XXX-XXX-XXX");
  ASSERT_EQ(refresh_token, "refresh-XXX-XXX-XXX-XXX");
}

TEST_F(GeminiJSONParserTest, GetTickerPriceFromJSON) {
  std::string price;
  ASSERT_TRUE(GeminiJSONParser::GetTickerPriceFromJSON(R"(
      {
        "bid":"0.25856",
        "ask":"0.25898",
        "volume":{
          "BAT":"199028.19240322",
          "USD":"51305.74053634907",
          "timestamp":1594605300000
        },
        "last":"0.25884"
      })", &price));
  ASSERT_EQ(price, "0.25856");
}

TEST_F(GeminiJSONParserTest, GetDepositInfoFromJSON) {
  std::string deposit_address;
  ASSERT_TRUE(GeminiJSONParser::GetDepositInfoFromJSON(R"(
      {
        "data": [
          {
            "address" : "n2saq73aDTu42bRgEHd8gd4to1gCzHxrdj",
            "timestamp" : 1424285102000,
            "label" : "my bitcoin address"
          }
        ]
      })", &deposit_address));
  ASSERT_EQ(deposit_address, "n2saq73aDTu42bRgEHd8gd4to1gCzHxrdj");
}

TEST_F(GeminiJSONParserTest, GetOrderQuoteInfoFromJSON) {
  std::string fee;
  std::string quote_id;
  std::string quantity;
  std::string price;
  std::string total_price;
  std::string error;
  ASSERT_TRUE(GeminiJSONParser::GetOrderQuoteInfoFromJSON(R"(
      {
        "data": {
          "quoteId": 1328,
          "maxAgeMs": 60000,
          "pair": "BTCUSD",
          "price": "6445.07",
          "priceCurrency": "USD",
          "side": "buy",
          "quantity": "0.01505181",
          "quantityCurrency": "BTC",
          "fee": "2.9900309233",
          "feeCurrency": "USD",
          "depositFee": "0",
          "depositFeeCurrency": "USD",
          "totalSpend": "100",
          "totalSpendCurrency": "USD"
        }
      })", &quote_id, &quantity, &fee, &price, &total_price, &error));
  ASSERT_EQ(quote_id, "1328");
  ASSERT_EQ(price, "6445.07");
  ASSERT_EQ(fee, "2.9900309233");
  ASSERT_EQ(quantity, "0.01505181");
  ASSERT_EQ(total_price, "100");
}

}  // namespace
