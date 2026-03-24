/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_time_period_storage_pref_util_internal.h"

#include <string_view>

#include "base/time/time.h"
#include "base/time/time_delta_from_string.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_keyword_path_component_util.h"

namespace brave_ads {

namespace {

constexpr std::string_view kTimePeriodStoragePathComponent =
    "time_period_storage";
constexpr std::string_view kTimePeriodStorageAllDuration = "all";

}  // namespace

std::optional<base::Time> MaybeResolveTimePeriodStorageCutoff(
    std::string_view path_component) {
  std::optional<std::string_view> value = MaybeParseKeywordPathComponentValue(
      path_component, kTimePeriodStoragePathComponent);
  if (!value) {
    // Not a time period storage path component.
    return std::nullopt;
  }

  if (value->empty() || value == kTimePeriodStorageAllDuration) {
    // No duration or "all" specified, so every event should count.
    return base::Time();
  }

  // `base::TimeDeltaFromString` accepts suffixed strings such as `7d`, `24h`,
  // `30m`, and `60s`; unit tests cover each suffix.
  if (std::optional<base::TimeDelta> time_delta =
          base::TimeDeltaFromString(*value)) {
    return base::Time::Now() - *time_delta;
  }

  // Invalid duration string.
  return std::nullopt;
}

double SumTimePeriodStorageListValues(const base::ListValue& list,
                                      base::Time cutoff) {
  double sum = 0.0;
  for (const auto& entry : list) {
    const base::DictValue* const dict = entry.GetIfDict();
    if (!dict) {
      continue;
    }

    // `TimePeriodStorage` serialises `day` as a double via
    // `base::Time::InSecondsFSinceUnixEpoch`, so round trips may drift by
    // about 1 µs; cutoffs are whole seconds so this is harmless.
    std::optional<double> day = dict->FindDouble("day");
    if (!day || base::Time::FromSecondsSinceUnixEpoch(*day) < cutoff) {
      continue;
    }

    if (std::optional<double> value = dict->FindDouble("value")) {
      sum += *value;
    }
  }
  return sum;
}

}  // namespace brave_ads
