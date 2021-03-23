/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_response_parser.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"

namespace {

bool ParseSingleStringResult(const std::string& json, std::string* result) {
  DCHECK(result);
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

  return response_dict->GetString("result", result);
}

}  // namespace

namespace brave_wallet {

bool ParseEthGetBalance(const std::string& json, std::string* hex_balance) {
  return ParseSingleStringResult(json, hex_balance);
}

bool ParseEthCall(const std::string& json, std::string* result) {
  return ParseSingleStringResult(json, result);
}

}  // namespace brave_wallet
