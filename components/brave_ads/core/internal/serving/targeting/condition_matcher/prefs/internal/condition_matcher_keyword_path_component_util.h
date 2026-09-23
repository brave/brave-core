/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_KEYWORD_PATH_COMPONENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_KEYWORD_PATH_COMPONENT_UTIL_H_

#include <optional>
#include <string_view>

// Parses "keyword[=value]" path components used in condition matcher pref
// paths, allowing each keyword to carry an optional value after '='.

namespace brave_ads {

// Parses a "keyword[=value]" path component and returns the value portion,
// i.e. the text after '=', or an empty string_view if no '=' is present.
// Returns `std::nullopt` if `path_component` does not start with `keyword` or
// has trailing content that is not preceded by '='.
std::optional<std::string_view> MaybeParseKeywordPathComponentValue(
    std::string_view path_component,
    std::string_view keyword);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_KEYWORD_PATH_COMPONENT_UTIL_H_
