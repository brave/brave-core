/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/binance/browser/binance_json_parser.h"

#include "brave/components/binance/browser/binance_service.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"

// npm run test -- brave_unit_tests --filter=BinanceJSONParserTest.*

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

std::vector<double> GetVectorFromDoubleMap(
    const std::map<std::string, std::vector<double>>& map,
    const std::string& key) {
  std::vector<double> value;
  std::map<std::string, std::vector<double>>::const_iterator it =
      map.find(key);
  if (it != map.end()) {
    value = it->second;
  }
  return value;
}

std::vector<BinanceConvertSubAsset> GetSubAssetsFromBinanceConvertAssets(
    const BinanceConvertAsserts& map,
    const std::string& key) {
  std::vector<BinanceConvertSubAsset> value;
  BinanceConvertAsserts::const_iterator it =
      map.find(key);
  if (it != map.end()) {
    value = it->second;
  }
  return value;
}

typedef testing::Test BinanceJSONParserTest;

TEST_F(BinanceJSONParserTest, GetAccountBalancesFromJSON) {
  BinanceAccountBalances balances;
  ASSERT_TRUE(BinanceJSONParser::GetAccountBalancesFromJSON(R"(
      {
        "code": "000000",
        "message": null,
        "data": [
          {
            "asset": "BNB",
            "free": 10114.00000000,
            "locked": "0.00000000",
            "freeze": "999990.00000000",
            "withdrawing": "0.00000000",
            "btcValuation": 2.000000,
            "fiatValuation": 17.500000
          },
          {
            "asset": "BTC",
            "free": 2.45000000,
            "locked": "0.00000000",
            "freeze": "999990.00000000",
            "withdrawing": "0.00000000",
            "btcValuation": 2.45000000,
            "fiatValuation": 20000.0000
          }
        ]
      })", &balances));

  std::vector<double>
      bnb_balance = GetVectorFromDoubleMap(balances, "BNB");
  std::vector<double>
      btc_balance = GetVectorFromDoubleMap(balances, "BTC");

  const uint64_t three = 3;

  ASSERT_EQ(bnb_balance.size(), three);
  ASSERT_EQ(btc_balance.size(), three);

  ASSERT_EQ(bnb_balance[0], 10114.00000000);
  ASSERT_EQ(btc_balance[0], 2.45000000);

  ASSERT_EQ(bnb_balance[1], 2.000000);
  ASSERT_EQ(btc_balance[1], 2.45000000);

  ASSERT_EQ(bnb_balance[2], 17.500000);
  ASSERT_EQ(btc_balance[2], 20000.0000);
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

TEST_F(BinanceJSONParserTest, GetDepositInfoFromJSON) {
  std::string deposit_address;
  std::string deposit_tag;
  ASSERT_TRUE(BinanceJSONParser::GetDepositInfoFromJSON(R"(
      {
        "code": "0000",
        "message": "null",
        "data": {
          "coin": "BTC",
          "tag": "",
          "address": "112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW",
          "url": "https://btc.com/112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW",
          "time": 1566366289000
        }
      })", &deposit_address, &deposit_tag));
  ASSERT_EQ(deposit_address, "112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW");
  ASSERT_EQ(deposit_tag, "");
}

TEST_F(BinanceJSONParserTest, GetDepositInfoFromJSONWithTag) {
  std::string deposit_address;
  std::string deposit_tag;
  ASSERT_TRUE(BinanceJSONParser::GetDepositInfoFromJSON(R"(
      {
        "code": "0000",
        "message": "null",
        "data": {
          "coin": "EOS",
          "tag": "0902394082",
          "address": "binancecleos",
          "url": "",
          "time": 1566366289000
        }
      })", &deposit_address, &deposit_tag));
  ASSERT_EQ(deposit_address, "binancecleos");
  ASSERT_EQ(deposit_tag, "0902394082");
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
          "quotePrice": "1094.01086957",
          "tradeFee": "8.000000",
          "railFee": "0",
          "totalFee": "8.000000",
          "totalAmount": "100649.010000",
          "showPrice": "1094.01086957"
        }
      })", &quote_id, &quote_price, &total_fee, &total_amount));
  ASSERT_EQ(quote_id, "b5481fb7f8314bb2baf55aa6d4fcf068");
  ASSERT_EQ(quote_price, "1094.01086957");
  ASSERT_EQ(total_fee, "8.000000");
  ASSERT_EQ(total_amount, "100649.010000");
}

TEST_F(BinanceJSONParserTest, GetConfirmStatusFromJSONSuccess) {
  std::string error;
  bool success;
  ASSERT_TRUE(BinanceJSONParser::GetConfirmStatusFromJSON(R"(
      {
        "code": "000000",
        "message": null,
        "data": {
            "quoteId": "b5481fb7f8314bb2baf55aa6d4fcf068",
            "status": "WAIT_MARKET",
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
        },
        "success": true
      })", &error, &success));
  ASSERT_EQ(error, "");
  ASSERT_TRUE(success);
}

TEST_F(BinanceJSONParserTest, GetConfirmStatusFromJSONFail) {
  std::string error;
  bool success;
  ASSERT_TRUE(BinanceJSONParser::GetConfirmStatusFromJSON(R"(
      {
        "code": "117041",
        "message": "Quote expired. Please try again.",
        "data": null,
        "success": false
      })", &error, &success));
  ASSERT_EQ(error, "Quote expired. Please try again.");
  ASSERT_FALSE(success);
}

TEST_F(BinanceJSONParserTest, RevokeTokenFromJSONSuccess) {
  bool success;
  ASSERT_TRUE(BinanceJSONParser::RevokeTokenFromJSON(R"(
      {
        "code": "000000",
        "message": null,
        "data": true,// true means clear access_token success
        "success": true
      })", &success));
  ASSERT_TRUE(success);
}

TEST_F(BinanceJSONParserTest, RevokeTokenFromJSONFail) {
  bool success;
  ASSERT_TRUE(BinanceJSONParser::RevokeTokenFromJSON(R"(
      {
        "code": "000000",
        "message": null,
        "data": false,// true means clear access_token success
        "success": false
      })", &success));
  ASSERT_FALSE(success);
}

TEST_F(BinanceJSONParserTest, GetCoinNetworksFromJSON) {
  BinanceCoinNetworks networks;
  ASSERT_TRUE(BinanceJSONParser::GetCoinNetworksFromJSON(R"(
      {
        "code": "000000",
        "message": null,
        "data": [
          {
            "coin": "BAT",
            "networkList": [
              {
                "coin": "BAT",
                "network": "ETH",
                "isDefault": true
              },
              {
                "coin": "BAT",
                "network": "BNB",
                "isDefault": false
              }
            ]
          },
          {
            "coin": "GAS",
            "networkList": [
              {
                "coin": "GAS",
                "network": "BTC",
                "isDefault": false
              },
              {
                "coin": "GAS",
                "network": "NEO",
                "isDefault": true
              }
            ]
          }
        ]
      })", &networks));

  std::string bat_network = GetValueFromStringMap(networks, "BAT");
  std::string gas_network = GetValueFromStringMap(networks, "GAS");
  ASSERT_EQ(bat_network, "ETH");
  ASSERT_EQ(gas_network, "NEO");
}

TEST_F(BinanceJSONParserTest, GetConvertAssetsFromJSON) {
  BinanceConvertAsserts assets;

  ASSERT_TRUE(BinanceJSONParser::GetConvertAssetsFromJSON(R"(
      {
        "code":"000000",
        "message":null,
        "data":[{
          "assetCode":"BTC",
          "assetName":"Bitcoin",
          "logoUrl":"https://bin.bnbstatic.com/images/20191211/fake.png",
          "size":"6",
          "order":0,
            "freeAsset":"0.00508311",
            "subSelector":[
              {
                "assetCode":"BNB",
                "assetName":"BNB",
                "logoUrl":"https://bin.bnbstatic.com/images/fake.png",
                "size":"2",
                "order":1,
                "perTimeMinLimit":0.00200000,
                "perTimeMaxLimit":1.00000000,
                "dailyMaxLimit":10.00000000,
                "hadDailyLimit":"0",
                "needMarket":true,
                "feeType":1,
                "feeRate":0.00050000,
                "fixFee":"1.00000000",
                "feeCoin":"BTC",
                "forexRate":1.00000000,
                "expireTime":30
              },
              {
                "assetCode":"ETH",
                "assetName":"ETH",
                "logoUrl":"https://bin.bnbstatic.com/images/fake.png",
                "size":"2",
                "order":1,
                "perTimeMinLimit":0.00500000,
                "perTimeMaxLimit":1.00000000,
                "dailyMaxLimit":10.00000000,
                "hadDailyLimit":"0",
                "needMarket":true,
                "feeType":1,
                "feeRate":0.00050000,
                "fixFee":"1.00000000",
                "feeCoin":"BTC",
                "forexRate":1.00000000,
                "expireTime":30
              }
            ]
        }]
      })", &assets));
  std::vector<BinanceConvertSubAsset> sub =
      GetSubAssetsFromBinanceConvertAssets(assets, "BTC");

  BinanceConvertSubAsset bnb_sub = sub.front();
  std::string bnb_name = bnb_sub.assetName;
  double bnb_min = bnb_sub.minAmount;
  ASSERT_EQ(bnb_name, "BNB");
  ASSERT_EQ(bnb_min, 0.00200000);

  BinanceConvertSubAsset eth_sub = sub.back();
  std::string eth_name = eth_sub.assetName;
  double eth_min = eth_sub.minAmount;
  ASSERT_EQ(eth_name, "ETH");
  ASSERT_EQ(eth_min, 0.00500000);
}

}  // namespace
