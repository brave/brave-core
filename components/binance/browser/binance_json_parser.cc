/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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
//  "expires_in": 30714
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
//   "code": "XXXX",
//   "data": {
//     ...
//   }
// }
bool BinanceJSONParser::GetQuoteIDFromJSON(
    const std::string& json, std::string *quote_id) {
  if (!quote_id) {
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

  const base::Value* id = records_v->FindKey("code");

  if (id && id->is_string()) {
    *quote_id = id->GetString();
  }

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
