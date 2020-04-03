/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/components/binance/browser/binance_json_parser.h"

#include "base/json/json_reader.h"

// static
//
// Response Format
//
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
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;
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
//
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
//            "withdrawing": "0.00000000"
//        }
//    ]
// }
//
bool BinanceJSONParser::GetAccountBalancesFromJSON(
    const std::string& json, std::map<std::string, std::string>* balances) {
  if (!balances) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* pv_arr = records_v->FindKey("data");
  if (pv_arr && pv_arr->is_list()) {
    for (const base::Value &val : pv_arr->GetList()) {
      const base::Value* asset = val.FindKey("asset");
      const base::Value* free_amount = val.FindKey("free");
      const base::Value* locked_amount = val.FindKey("locked");
      if (asset && asset->is_string() &&
          free_amount && free_amount->is_string() &&
          locked_amount && locked_amount->is_string()) {
        std::string asset_symbol = asset->GetString();
        balances->insert({asset_symbol, free_amount->GetString()});
      }
    }
  }

  return true;
}

// static
//
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "data": {
//      "quoteId": "b5481fb7f8314bb2baf55aa6d4fcf068",
//      "quotePrice": 1094.01086957,
//      "tradeFee": 8,
//      "railFee": 0,
//      "totalFee": 8,
//      "totalAmount": 100649,
//      "showPrice": 1094.01086957
//    },
// }
bool BinanceJSONParser::GetQuoteInfoFromJSON(
    const std::string& json, std::string *quote_id,
    std::string *quote_price, std::string *total_fee,
    std::string *total_amount) {
  if (!quote_id || !quote_price ||
      !total_fee || !total_amount) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;
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

  std::string id;
  double fee;
  double price;
  double amount;

  data_dict->GetString("quoteId", &id);
  data_dict->GetDouble("quotePrice", &price);
  data_dict->GetDouble("totalFee", &fee);
  data_dict->GetDouble("totalAmount", &amount);

  if (id.empty() || !price || !fee || !amount) {
    return false;
  }

  fee = static_cast<int>(fee);
  price = static_cast<int>(price);
  amount = static_cast<int>(amount);

  *quote_id = id;
  *quote_price = std::to_string(price);
  *total_fee = std::to_string(fee);
  *total_amount = std::to_string(amount);
  return true;
}

// static
bool BinanceJSONParser::GetTickerPriceFromJSON(
    const std::string& json, std::string* symbol_pair_price) {
  if (!symbol_pair_price) {
    return false;
  }
  // Response format:
  // {
  //   "symbol": "BTCUSDT",
  //   "price": "7137.98000000"
  // }
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& parsed_response = value_with_error.value;
  if (!parsed_response) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* price = parsed_response->FindKey("price");
  if (!price || !price->is_string()) {
    return false;
  }

  *symbol_pair_price = price->GetString();
  return true;
}

// static
bool BinanceJSONParser::GetTickerVolumeFromJSON(
    const std::string& json, std::string* symbol_pair_volume) {
  if (!symbol_pair_volume) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& parsed_response = value_with_error.value;
  if (!parsed_response) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* volume = parsed_response->FindKey("volume");
  if (!volume || !volume->is_string()) {
    return false;
  }

  *symbol_pair_volume = volume->GetString();
  return true;
}

// static
//
// Response Format:
//
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
//
bool BinanceJSONParser::GetDepositInfoFromJSON(
    const std::string& json, std::string *address, std::string *url) {
  if (!address || !url) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;

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

  std::string deposit_url;
  std::string deposit_address;

  if (!data_dict->GetString("url", &deposit_url) ||
      !data_dict->GetString("address", &deposit_address)) {
    return false;
  }

  *url = deposit_url;
  *address = deposit_address;
  return true;
}

// static
//
// Response Format:
//
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
//    }
// }
bool BinanceJSONParser::GetConfirmStatusFromJSON(
    const std::string& json, std::string *success_status) {
  if (!success_status) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;
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

  std::string success;

  if (!data_dict->GetString("status", &success)) {
    return false;
  }

  *success_status = success;
  return true;
}

// static
bool BinanceJSONParser::GetConvertAssetsFromJSON(const std::string& json,
    std::map<std::string, std::vector<std::string>>* assets) {
  if (!assets) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* data_arr = records_v->FindKey("data");
  if (data_arr && data_arr->is_list()) {
    for (const base::Value &val : data_arr->GetList()) {
      const base::Value* asset_code = val.FindKey("assetCode");
      if (asset_code && asset_code->is_string()) {
        std::vector<std::string> sub_selectors;
        std::string asset_symbol = asset_code->GetString();
        const base::Value* selectors = val.FindKey("subSelector");
        if (selectors && selectors->is_list()) {
          for (const base::Value &selector : selectors->GetList()) {
            const base::Value* sub_code = selector.FindKey("assetCode");
            if (sub_code && sub_code->is_string()) {
              sub_selectors.push_back(sub_code->GetString());
            }
          }
          assets->insert({asset_symbol, sub_selectors});
        }
      }
    }
  }

  return true;
}
