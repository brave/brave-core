/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics.h"

#include <algorithm>
#include <string>

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "components/prefs/pref_service.h"

namespace metrics {

namespace {

constexpr char kBraveSearchEngineDictKey[] = "brave_search_engine";
constexpr char kGoogleSearchEngineDictKey[] = "google_search_engine";
constexpr char kOtherSearchEngineDictKey[] = "other_search_engine";

// Returns the start of yesterday (local time), i.e., midnight at the beginning
// of the previous day.
base::Time GetStartOfYesterday() {
  return base::Time::Now().LocalMidnight() - base::Days(1);
}

// Returns the end of yesterday (local time), i.e., one millisecond before
// today's midnight. This ensures the time range for 'yesterday' is inclusive up
// to the very end of the day.
base::Time GetEndOfYesterday() {
  return base::Time::Now().LocalMidnight() - base::Milliseconds(1);
}

// Returns the end of the stale period (local time), which is one millisecond
// before the start of yesterday. Used to define a cutoff for older data.
base::Time GetEndOfStalePeriod() {
  return GetStartOfYesterday() - base::Milliseconds(1);
}

size_t GetYesterdaySumAfterLastCheckedCutoff(const TimePeriodStorage& storage,
                                             base::Time start_of_yesterday,
                                             base::Time end_of_yesterday,
                                             base::Time start_of_stale_period) {
  if (!start_of_stale_period.is_null()) {
    start_of_yesterday = std::max(start_of_yesterday, start_of_stale_period);
  }

  if (start_of_yesterday > end_of_yesterday) {
    return 0;
  }

  return storage.GetPeriodSumInTimeRange(start_of_yesterday, end_of_yesterday);
}

}  // namespace

SerpMetrics::SerpMetrics(PrefService* local_state)
    : local_state_(local_state),
      brave_search_engine_time_period_storage_(
          local_state,
          prefs::kSerpMetricsTimePeriodStorage,
          kBraveSearchEngineDictKey,
          kSerpMetricsTimePeriodInDays.Get(),
          /*should_offset_dst=*/false),
      google_search_engine_time_period_storage_(
          local_state,
          prefs::kSerpMetricsTimePeriodStorage,
          kGoogleSearchEngineDictKey,
          kSerpMetricsTimePeriodInDays.Get(),
          /*should_offset_dst=*/false),
      other_search_engine_time_period_storage_(
          local_state,
          prefs::kSerpMetricsTimePeriodStorage,
          kOtherSearchEngineDictKey,
          kSerpMetricsTimePeriodInDays.Get(),
          /*should_offset_dst=*/false) {
  CHECK(local_state_);
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
    return base::Time();
  }

  base::Time last_checked_at;
  const bool success =
      base::Time::FromString(last_check_ymd.c_str(), &last_checked_at);
  if (!success) {
    // If we can't parse the last check date, assume the full time period.
    return base::Time();
  }

  // Return the day after the last checked date.
  return last_checked_at.LocalMidnight() + base::Days(1);
}

}  // namespace metrics
