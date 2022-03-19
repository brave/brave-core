/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/components/binance/browser/binance_json_parser.h"

#include "base/json/json_reader.h"
#include "base/logging.h"

// static
// Response Format
// {
//   "access_token": "83f2bf51-a2c4-4c2e-b7c4-46cef6a8dba5",
//   "refresh_token": "fb5587ee-d9cf-4cb5-a586-4aed72cc9bea",
//   "scope": "read",
//   "token_type": "bearer",
//   "expires_in": 30714
// }
//
bool BinanceJSONParser::GetTokensFromJSON(
    const std::string& json, std::string *value, std::string type) {
  if (!value) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* token = records_v->FindKey(type);

  if (token && token->is_string()) {
    *value = token->GetString();
  }

  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "data": [
//        {
//            "asset": "ADA",
//            "free": "0.00000000",
//            "locked": "0.00000000",
//            "freeze": "1.00000000",
//            "withdrawing": "0.00000000",
//            "btcValuation": "0.00000000",
//            "fiatValuation": "0.00000000"
//        }
//    ]
// }
//
bool BinanceJSONParser::GetAccountBalancesFromJSON(
    const std::string& json, BinanceAccountBalances* balances) {
  if (!balances) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* pv_arr = records_v->FindKey("data");
  if (!pv_arr || !pv_arr->is_list()) {
    return false;
  }

  for (const base::Value& val : pv_arr->GetList()) {
    const base::Value* asset = val.FindKey("asset");
    const base::Value* free_amount = val.FindKey("free");
    const base::Value* btc_val = val.FindKey("btcValuation");
    const base::Value* fiat_val = val.FindKey("fiatValuation");

    bool has_asset = asset && asset->is_string();
    bool has_free = free_amount && free_amount->is_string();
    bool has_btc_val = btc_val && btc_val->is_string();
    bool has_fiat_val = fiat_val && fiat_val->is_string();

    if (!has_asset || !has_free || !has_btc_val || !has_fiat_val) {
      continue;
    }

    std::vector<std::string> balance_data;
    balance_data.push_back(free_amount->GetString());
    balance_data.push_back(btc_val->GetString());
    balance_data.push_back(fiat_val->GetString());

    balances->insert({asset->GetString(), balance_data});
  }

  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "data": {
//      "quoteId": "b5481fb7f8314bb2baf55aa6d4fcf068",
//      "quotePrice": "1094.01086957",
//      "tradeFee": "8",
//      "railFee": "0",
//      "totalFee": "8",
//      "totalAmount": "100649",
//      "showPrice": "1094.01086957"
//    },
// }
bool BinanceJSONParser::GetQuoteInfoFromJSON(
    const std::string& json, std::string *quote_id,
    std::string *quote_price, std::string *total_fee,
    std::string *total_amount) {
  DCHECK(quote_id);
  DCHECK(quote_price);
  DCHECK(total_fee);
  DCHECK(total_amount);

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  const base::DictionaryValue* data_dict;
  if (!response_dict->GetDictionary("data", &data_dict)) {
    return false;
  }

  if (!data_dict->GetString("quoteId", quote_id) ||
      !data_dict->GetString("quotePrice", quote_price) ||
      !data_dict->GetString("totalFee", total_fee) ||
      !data_dict->GetString("totalAmount", total_amount)) {
    return false;
  }

  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "data": {
//      "coin": "BTC",
//      "address": "112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW",
//      "tag": "",
//      "url": "https://btc.com/112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW",
//      "time": 1566366289000
//    },
//    "success": true
// }
bool BinanceJSONParser::GetDepositInfoFromJSON(
    const std::string& json, std::string *address, std::string *tag) {
  DCHECK(address);
  DCHECK(tag);

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  const base::DictionaryValue* data_dict;
  if (!response_dict->GetDictionary("data", &data_dict)) {
    return false;
  }

  if (!data_dict->GetString("tag", tag) ||
      !data_dict->GetString("address", address)) {
    return false;
  }

  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "data": {
//        "quoteId": "b5481fb7f8314bb2baf55aa6d4fcf068",
//        "status": "FAIL",
//        "orderId": "ab0ab6cfd62240d79e10347fc5000bc4",
//        "fromAsset": "BNB",
//        "toAsset": "TRX",
//        "sourceAmount": 100,
//        "obtainAmount": 100649,
//        "tradeFee": 8,
//        "price": 1094.01086957,
//        "feeType": 1,
//        "feeRate": 0.08000000,
//        "fixFee": 13.00000000
//    },
//    "success": true
// }
bool BinanceJSONParser::GetConfirmStatusFromJSON(
    const std::string& json, std::string *error_message,
    bool* success_status) {
  if (!error_message || !success_status) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  const base::DictionaryValue* data_dict;
  if (!response_dict->GetDictionary("data", &data_dict)) {
    std::string message;
    if (!response_dict->GetString("message", &message)) {
      return false;
    }
    *success_status = false;
    *error_message = message;
    return true;
  }

  *success_status = true;
  return true;
}

// static
// Response Format:
// {
//   "code":"000000",
//   "message":null,
//   "data":[{
//     "assetCode":"BTC",
//     "assetName":"Bitcoin",
//     "logoUrl":"https://bin.bnbstatic.com/images/20191211/fake.png",
//     "size":"6",
//     "order":0,
//     "freeAsset":"0.00508311",
//     "subSelector":[{
//       "assetCode":"BNB",
//       "assetName":"BNB",
//       "logoUrl":"https://bin.bnbstatic.com/images/fake.png",
//       "size":"2",
//       "order":1,
//       "perTimeMinLimit":"0.00200000",
//       "perTimeMaxLimit":"1.00000000",
//       "dailyMaxLimit":"10.00000000",
//       "hadDailyLimit":"0",
//       "needMarket":true,
//       "feeType":1,
//       "feeRate":"0.00050000",
//       "fixFee":"1.00000000",
//       "feeCoin":"BTC",
//       "forexRate":"1.00000000",
//       "expireTime":30
//     }]
//   }],
//   "success":true
// }
bool BinanceJSONParser::GetConvertAssetsFromJSON(const std::string& json,
    BinanceConvertAsserts* assets) {
  if (!assets) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* data_arr = records_v->FindKey("data");
  if (data_arr && data_arr->is_list()) {
    for (const base::Value& val : data_arr->GetList()) {
      const base::Value* asset_code = val.FindKey("assetCode");
      if (asset_code && asset_code->is_string()) {
        std::vector<std::map<std::string, std::string>> sub_selectors;
        std::string asset_symbol = asset_code->GetString();
        const base::Value* selectors = val.FindKey("subSelector");
        if (selectors && selectors->is_list()) {
          for (const base::Value& selector : selectors->GetList()) {
            std::map<std::string, std::string> sub_selector;
            const base::Value* sub_code = selector.FindKey("assetCode");
            const base::Value* min_limit = selector.FindKey("perTimeMinLimit");
            if (sub_code && sub_code->is_string() &&
                min_limit && min_limit->is_string()) {
              sub_selector.insert({"asset", sub_code->GetString()});
              sub_selector.insert({"minAmount", min_limit->GetString()});
            }
            sub_selectors.push_back(sub_selector);
          }
          assets->insert({asset_symbol, sub_selectors});
        }
      }
    }
  }
  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "data": true,// true means clear access_token success
//    "success": true
// }
bool BinanceJSONParser::RevokeTokenFromJSON(
    const std::string& json,
    bool* success_status) {
  DCHECK(success_status);
  if (!success_status) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  absl::optional<bool> success_optional = response_dict->FindBoolKey("success");
  if (!success_optional.has_value())
    return false;

  *success_status = success_optional.value();
  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "messageDetail": null,
//    "success": true,
//    "data": [
//        {
//            "coin": "CTR",
//            "networkList": [
//                {
//                    "coin": "CTR",
//                    "network": "ETH"
//                }
//            ]
//        }
//    ]
// }
//
bool BinanceJSONParser::GetCoinNetworksFromJSON(
    const std::string& json, BinanceCoinNetworks* networks) {
  if (!networks) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* data_arr = records_v->FindKey("data");
  if (!data_arr || !data_arr->is_list()) {
    return false;
  }

  for (const base::Value& coin : data_arr->GetList()) {
    const base::Value* coin_name = coin.FindKey("coin");
    if (!coin_name || !coin_name->is_string()) {
      return false;
    }

    const base::Value* network_list = coin.FindKey("networkList");
    if (!network_list || !network_list->is_list()) {
      return false;
    }

    for (const base::Value& network : network_list->GetList()) {
      const base::Value* network_name = network.FindKey("network");
      const base::Value* is_default = network.FindKey("isDefault");
      const bool default_valid =
          is_default && is_default->is_bool() && is_default->GetBool();
      const bool network_name_valid =
          network_name && network_name->is_string();

      if (default_valid && network_name_valid) {
        networks->insert({coin_name->GetString(), network_name->GetString()});
        break;
      }
    }
  }

  return true;
}
