/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/components/gemini/browser/gemini_json_parser.h"

#include "base/json/json_reader.h"

// static
// Response Format
// {
//   "access_token": "xxx-xxx-xxx-xxx-xxx",
//   "refresh_token": "xxx-xxx-xxx-xxx-xxx",
//   "scope": "Trader",
//   "token_type": "Bearer",
//   "expires_in": 30714
// }
//
bool GeminiJSONParser::GetTokensFromJSON(
    const std::string& json, std::string *access_token,
    std::string* refresh_token) {
  if (!access_token || !refresh_token) {
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

  const base::Value* access = records_v->FindKey("access_token");
  const base::Value* refresh = records_v->FindKey("refresh_token");

  if (access && access->is_string()) {
    *access_token = access->GetString();
  }

  if (refresh && refresh->is_string()) {
    *refresh_token = refresh->GetString();
  }

  return true;
}

bool GeminiJSONParser::GetTickerPriceFromJSON(
    const std::string& json, std::string* price) {
  if (!price) {
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

  const base::Value* bid = records_v->FindKey("bid");

  if (bid && bid->is_string()) {
    *price = bid->GetString();
  }

  return true;
}

bool GeminiJSONParser::GetAccountBalancesFromJSON(
    const std::string& json,
    std::map<std::string, std::string>* balances) {
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
  if (!pv_arr || !pv_arr->is_list()) {
    return false;
  }

  for (const base::Value &val : pv_arr->GetList()) {
    const base::Value* currency = val.FindKey("currency");
    const base::Value* available = val.FindKey("available");

    bool has_currency = currency && currency->is_string();
    bool has_available = available && available->is_string();

    if (!has_currency || !has_available) {
      continue;
    }

    balances->insert({currency->GetString(), available->GetString()});
  }

  return true;
}

bool GeminiJSONParser::GetDepositInfoFromJSON(
    const std::string& json, std::string *address) {
  if (!address) {
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
  if (!pv_arr || !pv_arr->is_list() || pv_arr->GetList().size() == 0) {
    return false;
  }

  const base::Value &val = pv_arr->GetList()[0];
  const base::Value* asset_address = val.FindKey("address");

  if (asset_address && asset_address->is_string()) {
    *address = asset_address->GetString();
  }

  return true;
}
