/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_JSON_JSON_HELPER_H_
#define BRAVE_COMPONENTS_JSON_JSON_HELPER_H_

#include <string>

#include "brave/components/json/json_export.h"

namespace json {

JSON_HELPER_EXPORT std::string convert_uint64_value_to_string(
    const std::string& path,
    const std::string& json,
    bool optional);
JSON_HELPER_EXPORT std::string convert_int64_value_to_string(
    const std::string& path,
    const std::string& json,
    bool optional);
JSON_HELPER_EXPORT std::string convert_string_value_to_uint64(
    const std::string& path,
    const std::string& json,
    bool optional);
JSON_HELPER_EXPORT std::string convert_string_value_to_int64(
    const std::string& path,
    const std::string& json,
    bool optional);
JSON_HELPER_EXPORT std::string convert_uint64_in_object_array_to_string(
    const std::string& path_to_list,
    const std::string& path_to_object,
    const std::string& key,
    const std::string& json);

JSON_HELPER_EXPORT std::string convert_all_numbers_to_string(
    const std::string& json,
    const std::string& path);

JSON_HELPER_EXPORT std::string
convert_all_numbers_to_string_and_remove_null_values(const std::string& json,
                                                     const std::string& path);

}  // namespace json

#endif  // BRAVE_COMPONENTS_JSON_JSON_HELPER_H_
