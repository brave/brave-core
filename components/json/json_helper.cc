/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/json/json_helper.h"

#include "brave/components/json/rs/src/lib.rs.h"

namespace json {

std::string convert_uint64_value_to_string(const std::string& path,
                                           const std::string& json,
                                           bool optional) {
  return std::string(
      rust_json::convert_uint64_value_to_string(path, json, optional));
}

std::string convert_int64_value_to_string(const std::string& path,
                                          const std::string& json,
                                          bool optional) {
  return std::string(
      rust_json::convert_int64_value_to_string(path, json, optional));
}

std::string convert_string_value_to_uint64(const std::string& path,
                                           const std::string& json,
                                           bool optional) {
  return std::string(
      rust_json::convert_string_value_to_uint64(path, json, optional));
}

std::string convert_string_value_to_int64(const std::string& path,
                                          const std::string& json,
                                          bool optional) {
  return std::string(
      rust_json::convert_string_value_to_int64(path, json, optional));
}

std::string convert_uint64_in_object_array_to_string(
    const std::string& path_to_list,
    const std::string& path_to_object,
    const std::string& key,
    const std::string& json) {
  return std::string(rust_json::convert_uint64_in_object_array_to_string(
      path_to_list, path_to_object, key, json));
}

std::string convert_all_numbers_to_string(const std::string& json,
                                          const std::string& path) {
  return std::string(rust_json::convert_all_numbers_to_string(json, path));
}

std::string convert_all_numbers_to_string_and_remove_null_values(
    const std::string& json,
    const std::string& path) {
  return std::string(
      rust_json::convert_all_numbers_to_string_and_remove_null_values(json,
                                                                      path));
}

}  // namespace json
