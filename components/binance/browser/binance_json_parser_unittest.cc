/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/binance/browser/binance_json_parser.h"

#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"

// npm run test -- brave_unit_tests --filter=BinanceJSONParserTest.*

namespace {

std::string GetBalanceFromAssets(
    const std::map<std::string, std::string>& balances,
    const std::string& asset) {
  std::string balance;
  std::map<std::string, std::string>::const_iterator it =
      balances.find(asset);
  if (it != balances.end()) {
    balance = it->second;
  }
  return balance;
}

typedef testing::Test BinanceJSONParserTest;

TEST_F(BinanceJSONParserTest, GetAccountBalancesFromJSON) {
  std::map<std::string, std::string> balances;
  ASSERT_TRUE(BinanceJSONParser::GetAccountBalancesFromJSON(R"(
      {
        "code": "000000",
        "message": null,
        "data": [
          {
            "asset": "BNB",
            "free": "10114.00000000",
            "locked": "0.00000000",
            "freeze": "999990.00000000",
            "withdrawing": "0.00000000"
          },
          {
            "asset": "BTC",
            "free": "2.45000000",
            "locked": "0.00000000",
            "freeze": "999990.00000000",
            "withdrawing": "0.00000000"
          }
        ]
      })", &balances));

  std::string bnb_balance = GetBalanceFromAssets(balances, "BNB");
  std::string btc_balance = GetBalanceFromAssets(balances, "BTC");
  ASSERT_EQ(bnb_balance, "10114.00000000");
  ASSERT_EQ(btc_balance, "2.45000000");
}

TEST_F(BinanceJSONParserTest, GetTokensFromJSON) {
  std::string access_token;
  std::string refresh_token;

  // Tokens are taken from documentation, examples only
  ASSERT_TRUE(BinanceJSONParser::GetTokensFromJSON(R"(
      {
        "access_token": "83f2bf51-a2c4-4c2e-b7c4-46cef6a8dba5",
        "refresh_token": "fb5587ee-d9cf-4cb5-a586-4aed72cc9bea",
        "scope": "read",
        "token_type": "bearer",
        "expires_in": 30714
      })", &access_token, "access_token"));

  ASSERT_TRUE(BinanceJSONParser::GetTokensFromJSON(R"(
      {
        "access_token": "83f2bf51-a2c4-4c2e-b7c4-46cef6a8dba5",
        "refresh_token": "fb5587ee-d9cf-4cb5-a586-4aed72cc9bea",
        "scope": "read",
        "token_type": "bearer",
        "expires_in": 30714
      })", &refresh_token, "refresh_token"));

  ASSERT_EQ(access_token, "83f2bf51-a2c4-4c2e-b7c4-46cef6a8dba5");
  ASSERT_EQ(refresh_token, "fb5587ee-d9cf-4cb5-a586-4aed72cc9bea");
}

TEST_F(BinanceJSONParserTest, GetTickerPriceFromJSON) {
  std::string symbol_pair_price;
  ASSERT_TRUE(BinanceJSONParser::GetTickerPriceFromJSON(R"(
      {
        "symbol": "BTCUSDT",
        "price": "7137.98000000"
      })", &symbol_pair_price));
  ASSERT_EQ(symbol_pair_price, "7137.98000000");
}

TEST_F(BinanceJSONParserTest, GetTickerVolumeFromJSON) {
  std::string symbol_pair_volume;
  ASSERT_TRUE(BinanceJSONParser::GetTickerVolumeFromJSON(R"(
      {
        "symbol": "BTCUSDT",
        "volume": "99849.90399800"
      })", &symbol_pair_volume));
  ASSERT_EQ(symbol_pair_volume, "99849.90399800");
}

TEST_F(BinanceJSONParserTest, GetDepositInfoFromJSON) {
  std::string deposit_address;
  std::string deposit_url;
  ASSERT_TRUE(BinanceJSONParser::GetDepositInfoFromJSON(R"(
      {
        "code": "0000",
        "message": "null",
        "data": {
          "coin": "BTC",
          "address": "112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW",
          "url": "https://btc.com/112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW",
          "time": 1566366289000
        }
      })", &deposit_address, &deposit_url));
  ASSERT_EQ(deposit_address, "112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW");
  ASSERT_EQ(deposit_url, "https://btc.com/112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW");
}

TEST_F(BinanceJSONParserTest, GetQuoteInfoFromJSON) {
  std::string quote_id;
  std::string quote_price;
  std::string total_fee;
  std::string total_amount;
  ASSERT_TRUE(BinanceJSONParser::GetQuoteInfoFromJSON(R"(
      {
        "code": "000000",
        "message": null,
        "data": {
          "quoteId": "b5481fb7f8314bb2baf55aa6d4fcf068",
          "quotePrice": 1094.01086957,
          "tradeFee": 8,
          "railFee": 0,
          "totalFee": 8,
          "totalAmount": 100649,
          "showPrice": 1094.01086957
        }
      })", &quote_id, &quote_price, &total_fee, &total_amount));
  ASSERT_EQ(quote_id, "b5481fb7f8314bb2baf55aa6d4fcf068");
  ASSERT_EQ(quote_price, "1094.000000");
  ASSERT_EQ(total_fee, "8.000000");
  ASSERT_EQ(total_amount, "100649.000000");
}

TEST_F(BinanceJSONParserTest, GetConfirmStatusFromJSON) {
  std::string success;
  ASSERT_TRUE(BinanceJSONParser::GetConfirmStatusFromJSON(R"(
      {
        "code": "000000",
        "message": null,
        "data": {
            "quoteId": "b5481fb7f8314bb2baf55aa6d4fcf068",
            "status": "FAIL",
            "orderId": "ab0ab6cfd62240d79e10347fc5000bc4",
            "fromAsset": "BNB",
            "toAsset": "TRX",
            "sourceAmount": 100,
            "obtainAmount": 100649,
            "tradeFee": 8,
            "price": 1094.01086957,
            "feeType": 1,
            "feeRate": 0.08000000,
            "fixFee": 13.00000000
        }
      })", &success));
  ASSERT_EQ(success, "FAIL");
}

}  // namespace
