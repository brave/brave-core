/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_INTERNAL_EPOCH_OPERATOR_CONDITION_MATCHER_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_INTERNAL_EPOCH_OPERATOR_CONDITION_MATCHER_UTIL_INTERNAL_H_

#include <cstdint>
#include <optional>
#include <string_view>

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

inline constexpr char kEpochOperatorConditionMatcherPrefixPattern[] = "[?]:*";

// Parses a number of days from a condition.
std::optional<int> ParseDays(std::string_view condition);

// Returns `true` if a Unix epoch timestamp.
bool IsUnixEpochTimestamp(int64_t timestamp);

// Converts a Windows timestamp to a Unix timestamp.
int64_t WindowsToUnixEpoch(int64_t timestamp);

// Returns the time delta since a Unix or Windows timestamp or an ISO 8601
// formatted date and time.
base::TimeDelta TimeDeltaSinceEpoch(int64_t timestamp);

// Parses a time delta from a string.
std::optional<base::TimeDelta> ParseTimeDelta(std::string_view value);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_INTERNAL_EPOCH_OPERATOR_CONDITION_MATCHER_UTIL_INTERNAL_H_
