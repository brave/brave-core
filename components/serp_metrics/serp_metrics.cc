/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics.h"

#include <algorithm>
#include <string>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "components/prefs/pref_service.h"

namespace serp_metrics {

namespace {

constexpr char kBraveSearchEngineDictKey[] = "brave_search_engine";
constexpr char kGoogleSearchEngineDictKey[] = "google_search_engine";
constexpr char kOtherSearchEngineDictKey[] = "other_search_engine";

// Returns the start of yesterday in local time (midnight at the beginning of
// the previous calendar day). Subtracting 12 hours ensures we cross into the
// previous day even during daylight saving time transitions, so the final
// normalization to local midnight always resolves to the correct day.
base::Time GetStartOfYesterday() {
  return (base::Time::Now().LocalMidnight() - base::Hours(12)).LocalMidnight();
}

// Returns the end of yesterday in local time, defined as the final millisecond
// before today's local midnight. Subtracting one millisecond ensures the
// yesterday time range is inclusive of all events on that day without spilling
// into today.
base::Time GetEndOfYesterday() {
  return base::Time::Now().LocalMidnight() - base::Milliseconds(1);
}

// Returns the end of the stale period in local time, defined as the final
// millisecond before the start of yesterday. This establishes a clear boundary
// between stale metrics and yesterday's metrics without overlap.
base::Time GetEndOfStalePeriod() {
  return GetStartOfYesterday() - base::Milliseconds(1);
}

// Returns the sum of metrics recorded during yesterday (local time) that have
// not already been reported. The later of the start of yesterday and the start
// of the stale period is used as the cutoff to avoid double-counting previously
// reported metrics. If the resulting time range does not include any portion of
// yesterday, the function returns 0.
size_t GetYesterdaySumAfterLastCheckedCutoff(
    const TimePeriodStorage& time_period_storage,
    base::Time start_of_yesterday,
    base::Time end_of_yesterday,
    base::Time start_of_stale_period) {
  const base::Time start_time =
      !start_of_stale_period.is_null()
          ? std::max(start_of_yesterday, start_of_stale_period)
          : start_of_yesterday;
  if (start_time > end_of_yesterday) {
    return 0;
  }

  return time_period_storage.GetPeriodSumInTimeRange(start_time,
                                                     end_of_yesterday);
}

}  // namespace

SerpMetrics::SerpMetrics(PrefService* local_state, PrefService* prefs)
    : local_state_(local_state),
      prefs_(prefs),
      brave_search_engine_time_period_storage_(
          prefs,
          prefs::kSerpMetricsTimePeriodStorage,
          kBraveSearchEngineDictKey,
          kSerpMetricsTimePeriodInDays.Get(),
          /*should_offset_dst=*/false),
      google_search_engine_time_period_storage_(
          prefs,
          prefs::kSerpMetricsTimePeriodStorage,
          kGoogleSearchEngineDictKey,
          kSerpMetricsTimePeriodInDays.Get(),
          /*should_offset_dst=*/false),
      other_search_engine_time_period_storage_(
          prefs,
          prefs::kSerpMetricsTimePeriodStorage,
          kOtherSearchEngineDictKey,
          kSerpMetricsTimePeriodInDays.Get(),
          /*should_offset_dst=*/false) {
  CHECK(local_state_);
  CHECK(prefs_);
  CHECK(base::FeatureList::IsEnabled(serp_metrics::kSerpMetricsFeature));
}

SerpMetrics::~SerpMetrics() = default;

void SerpMetrics::RecordBraveSearch() {
  brave_search_engine_time_period_storage_.AddDelta(1);
}

size_t SerpMetrics::GetBraveSearchCountForYesterday() const {
  return GetYesterdaySumAfterLastCheckedCutoff(
      brave_search_engine_time_period_storage_, GetStartOfYesterday(),
      GetEndOfYesterday(), GetStartOfStalePeriod());
}

size_t SerpMetrics::GetBraveSearchCountForStalePeriod() const {
  return brave_search_engine_time_period_storage_.GetPeriodSumInTimeRange(
      GetStartOfStalePeriod(), GetEndOfStalePeriod());
}

void SerpMetrics::RecordGoogleSearch() {
  google_search_engine_time_period_storage_.AddDelta(1);
}

size_t SerpMetrics::GetGoogleSearchCountForYesterday() const {
  return GetYesterdaySumAfterLastCheckedCutoff(
      google_search_engine_time_period_storage_, GetStartOfYesterday(),
      GetEndOfYesterday(), GetStartOfStalePeriod());
}

size_t SerpMetrics::GetGoogleSearchCountForStalePeriod() const {
  return google_search_engine_time_period_storage_.GetPeriodSumInTimeRange(
      GetStartOfStalePeriod(), GetEndOfStalePeriod());
}

void SerpMetrics::RecordOtherSearch() {
  other_search_engine_time_period_storage_.AddDelta(1);
}

size_t SerpMetrics::GetOtherSearchCountForYesterday() const {
  return GetYesterdaySumAfterLastCheckedCutoff(
      other_search_engine_time_period_storage_, GetStartOfYesterday(),
      GetEndOfYesterday(), GetStartOfStalePeriod());
}

size_t SerpMetrics::GetOtherSearchCountForStalePeriod() const {
  return other_search_engine_time_period_storage_.GetPeriodSumInTimeRange(
      GetStartOfStalePeriod(), GetEndOfStalePeriod());
}

///////////////////////////////////////////////////////////////////////////////

base::Time SerpMetrics::GetStartOfStalePeriod() const {
  // `kLastCheckYMD` exists to track when the last daily usage ping was sent,
  // so we can compute how far back metrics should be considered stale.
  const std::string& last_check_ymd = local_state_->GetString(kLastCheckYMD);
  if (last_check_ymd.empty()) {
    // If never checked, assume the full time period.
    return {};
  }

  base::Time last_checked_at;
  const bool success =
      base::Time::FromString(last_check_ymd.c_str(), &last_checked_at);
  if (!success) {
    // If we can't parse the last check date, assume the full time period.
    return {};
  }

  // Return the day after the last checked date.
  return last_checked_at.LocalMidnight() + base::Days(1);
}

}  // namespace serp_metrics
