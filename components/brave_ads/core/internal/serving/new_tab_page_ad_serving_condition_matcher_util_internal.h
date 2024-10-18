/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NEW_TAB_PAGE_AD_SERVING_CONDITION_MATCHER_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NEW_TAB_PAGE_AD_SERVING_CONDITION_MATCHER_UTIL_INTERNAL_H_

#include <cstdint>
#include <string>
#include <string_view>

#include "brave/components/brave_ads/core/public/prefs/pref_provider_interface.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

// Converts a base::Value to a string representation if possible. Returns
// `std::nullopt` for unsupported types.
std::optional<std::string> ToString(const base::Value& value);

// Parses the number of days from a condition string.  Returns `std::nullopt` if
// the condition is malformed.
std::optional<int> ParseDays(std::string_view condition);

// Checks if a timestamp is a Unix epoch timestamp.
bool IsUnixEpochTimestamp(int64_t timestamp);

// Converts a Windows epoch timestamp to a Unix epoch timestamp.
int64_t WindowsToUnixEpoch(int64_t timestamp);

// Calculates the time delta since the Unix or Windows epoch for a given
// timestamp.
base::TimeDelta TimeDeltaSinceEpoch(int64_t timestamp);

// Matches a value against a condition using operators. Supports equality,
// greater than, and greater than or equal operators.
bool MatchOperator(std::string_view value, std::string_view condition);

// Matches a value against a regular expression condition.
bool MatchRegex(std::string_view value, std::string_view condition);

// Matches a value against a pattern condition.
bool MatchPattern(std::string_view value, std::string_view condition);

// Get the pref value from the provider for the given path. Handles nested
// dictionaries, lists, and dot-separated keys. `base::Value::Find*ByDottedPath`
// is not used because path keys can contain dots. Returns `std::nullopt` if the
// path is malformed or unknown. Path keys should be separated by `|`. Example
// `list|1` would return the second element of a list.
std::optional<base::Value> MaybeGetPrefValue(
    const PrefProviderInterface* pref_provider,
    const std::string& pref_path);

// Get the pref value as a string from the provider for the given path. Handles
// nested dictionaries, lists, and dot-separated keys.
// `base::Value::Find*ByDottedPath` is not used because path keys can contain
// dots. Returns `std::nullopt` if the path is malformed or unknown. Path keys
// should be separated by `|`. Example `list|1` would return the second element
// of a list.
std::optional<std::string> MaybeGetPrefValueAsString(
    const PrefProviderInterface* pref_provider,
    const std::string& pref_path);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NEW_TAB_PAGE_AD_SERVING_CONDITION_MATCHER_UTIL_INTERNAL_H_
