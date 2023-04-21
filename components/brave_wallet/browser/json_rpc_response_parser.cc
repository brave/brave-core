/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"

#include <utility>

#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/json/rs/src/lib.rs.h"

namespace brave_wallet {

absl::optional<std::string> ParseSingleStringResult(
    const base::Value& json_value) {
  auto result_v = ParseResultValue(json_value);
  if (!result_v)
    return absl::nullopt;

  const std::string* result_str = result_v->GetIfString();
  if (!result_str)
    return absl::nullopt;

  return *result_str;
}

absl::optional<std::vector<uint8_t>> ParseDecodedBytesResult(
    const base::Value& json_value) {
  auto result_v = ParseResultValue(json_value);
  if (!result_v)
    return absl::nullopt;

  const std::string* result_str = result_v->GetIfString();
  if (!result_str)
    return absl::nullopt;

  return PrefixedHexStringToBytes(*result_str);
}

absl::optional<base::Value> ParseResultValue(const base::Value& json_value) {
  if (!json_value.is_dict()) {
    return absl::nullopt;
  }

  auto response =
      json_rpc_responses::RPCResponse::FromValue(json_value.GetDict());
  if (!response || !response->result)
    return absl::nullopt;
  return std::move(*response->result);
}

absl::optional<base::Value::Dict> ParseResultDict(
    const base::Value& json_value) {
  auto result = ParseResultValue(json_value);
  if (!result || !result->is_dict())
    return absl::nullopt;

  return std::move(result->GetDict());
}

absl::optional<base::Value::List> ParseResultList(
    const base::Value& json_value) {
  auto result = ParseResultValue(json_value);
  if (!result || !result->is_list())
    return absl::nullopt;

  return std::move(result->GetList());
}

absl::optional<bool> ParseBoolResult(const base::Value& json_value) {
  auto result = ParseSingleStringResult(json_value);
  if (!result)
    return absl::nullopt;

  if (*result ==
      "0x0000000000000000000000000000000000000000000000000000000000000001") {
    return true;
  } else if (*result ==
             "0x000000000000000000000000000000000000000000000000000000000000000"
             "0") {
    return false;
  }

  return absl::nullopt;
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
    const std::string& path_to_list,
    const std::string& path_to_object,
    const std::vector<std::string>& keys,
    const std::string& json) {
  if (path_to_list.empty() || json.empty() || keys.empty())
    return absl::nullopt;

  std::string converted_json(json);
  for (const auto& key : keys) {
    if (key.empty())
      return absl::nullopt;
    converted_json = std::string(json::convert_uint64_in_object_array_to_string(
        path_to_list, path_to_object, key, converted_json));
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
