/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/json_converter_impl.h"

#include <utility>

#include "brave/components/json/rs/src/lib.rs.h"

namespace brave_wallet {

JsonConverterImpl::JsonConverterImpl() = default;
JsonConverterImpl::~JsonConverterImpl() = default;

void JsonConverterImpl::ConvertUint64ValueToString(
    const std::string& path, const std::string& json, bool optional, JsonConverterStringCallback callback) {
  std::move(callback).Run(std::string(
        json::convert_uint64_value_to_string(path, json, optional)));
}

void JsonConverterImpl::ConvertInt64ValueToString(
    const std::string& path, const std::string& json, bool optional, JsonConverterStringCallback callback){
  std::move(callback).Run(std::string(
        json::convert_int64_value_to_string(path, json, optional)));
}

void JsonConverterImpl::ConvertStringValueToUint64(
    const std::string& path, const std::string& json, bool optional, JsonConverterStringCallback callback){
  std::move(callback).Run(std::string(
        json::convert_string_value_to_uint64(path, json, optional)));
}

void JsonConverterImpl::ConvertStringValueToInt64(
    const std::string& path, const std::string& json, bool optional, JsonConverterStringCallback callback){
  std::move(callback).Run(std::string(
        json::convert_string_value_to_int64(path, json, optional)));
}

void JsonConverterImpl::ConvertUint64InObjectArrayToString(
      const std::string& path_to_list, const std::string& path_to_object,
      const std::string& key, const std::string& json, JsonConverterStringCallback callback) {
  std::move(callback).Run(std::string(
        json::convert_uint64_in_object_array_to_string(path_to_list, path_to_object,
          key, json)));
}

void JsonConverterImpl::ConvertAllNumbersToString(
      const std::string& json, const std::string& path, JsonConverterStringCallback callback) {
  std::move(callback).Run(std::string(
        json::convert_all_numbers_to_string(json, path)));
}

}  // namespace brave_wallet
