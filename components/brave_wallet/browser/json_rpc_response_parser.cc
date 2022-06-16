/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"

#include <utility>

#include "brave/components/json/rs/src/lib.rs.h"

namespace brave_wallet {

bool ParseSingleStringResult(const std::string& json, std::string* result) {
  DCHECK(result);

  base::Value result_v;
  if (!ParseResult(json, &result_v))
    return false;

  const std::string* result_str = result_v.GetIfString();
  if (!result_str)
    return false;

  *result = *result_str;

  return true;
}

absl::optional<std::string> ParseSingleStringResult(const std::string& json) {
  std::string result;
  if (!ParseSingleStringResult(json, &result))
    return absl::nullopt;

  return result;
}

bool ParseResult(const std::string& json, base::Value* result) {
  DCHECK(result);
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

absl::optional<std::string> ConvertUint64ToString(const std::string& path,
                                                  const std::string& json) {
  if (path.empty() || json.empty())
    return absl::nullopt;

  std::string converted_json(
      json::convert_uint64_value_to_string(path, json, true));
  if (converted_json.empty())
    return absl::nullopt;

  return converted_json;
}

absl::optional<std::string> ConvertMultiUint64ToString(
    const std::vector<std::string>& paths,
    const std::string& json) {
  if (paths.empty() || json.empty())
    return absl::nullopt;

  std::string converted_json(json);
  for (const auto& path : paths) {
    auto result = ConvertUint64ToString(path, converted_json);
    if (!result)
      return absl::nullopt;
    converted_json = std::move(*result);
  }

  return converted_json;
}

absl::optional<std::string> ConvertMultiUint64InObjectArrayToString(
    const std::string& path,
    const std::vector<std::string>& keys,
    const std::string& json) {
  if (path.empty() || json.empty() || keys.empty())
    return absl::nullopt;

  std::string converted_json(json);
  for (const auto& key : keys) {
    if (key.empty())
      return absl::nullopt;
    converted_json = std::string(json::convert_uint64_in_object_array_to_string(
        path, key, converted_json));
    if (converted_json.empty())
      return absl::nullopt;
  }

  return converted_json;
}

absl::optional<std::string> ConvertInt64ToString(const std::string& path,
                                                 const std::string& json) {
  if (path.empty() || json.empty())
    return absl::nullopt;

  std::string converted_json(
      json::convert_int64_value_to_string(path, json, true));
  if (converted_json.empty())
    return absl::nullopt;
  return converted_json;
}

}  // namespace brave_wallet
