/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/rpc_response_parser.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

bool ParseSingleStringResult(const std::string& json, std::string* result) {
  DCHECK(result);

  base::Value result_v;
  if (!brave_wallet::ParseResult(json, &result_v))
    return false;

  const std::string* result_str = result_v.GetIfString();
  if (!result_str)
    return false;

  *result = *result_str;

  return true;
}

void ParseErrorResult(const std::string& json,
                      mojom::ProviderError* error,
                      std::string* error_message) {
  DCHECK(error);
  DCHECK(error_message);
  *error = mojom::ProviderError::kParsingError;
  *error_message = l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR);

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return;
  }

  absl::optional<int> code_int = response_dict->FindIntPath("error.code");
  const std::string* message_string =
      response_dict->FindStringPath("error.message");
  if (!code_int)
    return;

  *error = static_cast<mojom::ProviderError>(*code_int);
  if (message_string) {
    *error_message = *message_string;
  } else {
    error_message->clear();
  }
}

bool ParseResult(const std::string& json, base::Value* result) {
  DCHECK(result);
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  const base::Value* result_v = response_dict->FindPath("result");
  if (!result_v)
    return false;

  *result = result_v->Clone();

  return true;
}

bool ParseBoolResult(const std::string& json, bool* value) {
  DCHECK(value);

  std::string result;
  if (!ParseSingleStringResult(json, &result))
    return false;

  if (result ==
      "0x0000000000000000000000000000000000000000000000000000000000000001") {
    *value = true;
    return true;
  } else if (result ==
             "0x000000000000000000000000000000000000000000000000000000000000000"
             "0") {
    *value = false;
    return true;
  }

  return false;
}

}  // namespace brave_wallet
