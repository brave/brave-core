/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_TIME_PERIOD_STORAGE_PREF_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_TIME_PERIOD_STORAGE_PREF_UTIL_INTERNAL_H_

#include <optional>
#include <string_view>

namespace base {
class Time;
class ListValue;
}  // namespace base

// Resolves `TimePeriodStorage` pref values by aggregating event counts over a
// time window, all evaluated locally with nothing leaving the device.

namespace brave_ads {

// Resolves the cutoff time for a "time_period_storage[=<duration>]" condition
// so the caller can filter stored events to those within the requested window.
// Returns a distant-past cutoff when no duration or "all" is specified so every
// event counts; `now - duration` for a valid duration; or nullopt if
// `path_component` is not a `time_period_storage` keyword path component or the
// duration string is invalid.
std::optional<base::Time> MaybeResolveTimePeriodStorageCutoff(
    std::string_view path_component);

// Sums `value` fields from entries that are dicts containing both "day" and
// "value" double fields, where `day` is at or after `from`. Pass `base::Time()`
// to sum all entries regardless of age. "day" must be a Unix epoch timestamp in
// seconds, matching the format used by `TimePeriodStorage::Save` via
// `base::Time::InSecondsFSinceUnixEpoch`. Returns 0 if no entries match.
double SumTimePeriodStorageListValues(const base::ListValue& list,
                                      base::Time cutoff);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_TIME_PERIOD_STORAGE_PREF_UTIL_INTERNAL_H_
