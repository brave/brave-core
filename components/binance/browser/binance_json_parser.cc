/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/binance/browser/binance_json_parser.h"

#include "base/json/json_reader.h"

// static
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
