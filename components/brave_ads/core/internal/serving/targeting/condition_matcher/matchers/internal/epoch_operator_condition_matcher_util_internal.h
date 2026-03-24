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

// Internal helpers for epoch operator condition matching, exposed here so that
// each parsing step can be unit-tested independently.

namespace brave_ads {

// Parses the number of days from `condition`. `condition` must have been
// validated by `MaybeParseEpochOperatorType`. Returns `std::nullopt` if
// `condition` is malformed or the day count is negative.
std::optional<int> MaybeParseDays(std::string_view condition);

// Returns `true` if a Unix epoch timestamp.
bool IsUnixEpochTimestamp(int64_t timestamp);

// Converts a Windows timestamp to a Unix timestamp.
int64_t WindowsToUnixEpoch(int64_t timestamp);

// Returns the time delta since a Unix or Windows timestamp or an ISO 8601
// formatted date and time.
base::TimeDelta TimeDeltaSinceEpoch(int64_t timestamp);

// Parses `value` as a Unix or Windows epoch timestamp (numeric) or an ISO 8601
// date-time string, and returns the elapsed time since that point. Returns
// `std::nullopt` if `value` cannot be parsed as any supported format.
std::optional<base::TimeDelta> MaybeParseTimeDelta(std::string_view value);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_INTERNAL_EPOCH_OPERATOR_CONDITION_MATCHER_UTIL_INTERNAL_H_
