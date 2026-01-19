/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "components/prefs/pref_service.h"

namespace metrics {

namespace {

constexpr char kBraveSearchEngineDictKey[] = "brave_search_engine";
constexpr char kGoogleSearchEngineDictKey[] = "google_search_engine";
constexpr char kOtherSearchEngineDictKey[] = "other_search_engine";

constexpr size_t kTimePeriodInDays = 28;

}  // namespace

SerpMetrics::SerpMetrics(PrefService* local_state)
    : locale_state_(local_state),
      brave_search_engine_time_period_storage_(
          local_state,
          prefs::kSerpMetricsTimePeriodStorage,
          kBraveSearchEngineDictKey,
          kTimePeriodInDays),
      google_search_engine_time_period_storage_(
          local_state,
          prefs::kSerpMetricsTimePeriodStorage,
          kGoogleSearchEngineDictKey,
          kTimePeriodInDays),
      other_search_engine_time_period_storage_(
          local_state,
          prefs::kSerpMetricsTimePeriodStorage,
          kOtherSearchEngineDictKey,
          kTimePeriodInDays) {
  CHECK(locale_state_);
}

SerpMetrics::~SerpMetrics() = default;

void SerpMetrics::RecordBraveSearch() {
  brave_search_engine_time_period_storage_.AddDelta(1);
}

size_t SerpMetrics::GetBraveSearchCountForYesterday() const {
  return brave_search_engine_time_period_storage_.GetPeriodSumInTimeRange(
      GetStartOfYesterday(), GetEndOfYesterday());
}

size_t SerpMetrics::GetBraveSearchCountForStalePeriod() const {
  return brave_search_engine_time_period_storage_.GetPeriodSumInTimeRange(
      GetStartOfStalePeriod(), GetEndOfStalePeriod());
}

void SerpMetrics::RecordGoogleSearch() {
  google_search_engine_time_period_storage_.AddDelta(1);
}

size_t SerpMetrics::GetGoogleSearchCountForYesterday() const {
  return google_search_engine_time_period_storage_.GetPeriodSumInTimeRange(
      GetStartOfYesterday(), GetEndOfYesterday());
}

size_t SerpMetrics::GetGoogleSearchCountForStalePeriod() const {
  return google_search_engine_time_period_storage_.GetPeriodSumInTimeRange(
      GetStartOfStalePeriod(), GetEndOfStalePeriod());
}

void SerpMetrics::RecordOtherSearch() {
  other_search_engine_time_period_storage_.AddDelta(1);
}

size_t SerpMetrics::GetOtherSearchCountForYesterday() const {
  return other_search_engine_time_period_storage_.GetPeriodSumInTimeRange(
      GetStartOfYesterday(), GetEndOfYesterday());
}

size_t SerpMetrics::GetOtherSearchCountForStalePeriod() const {
  return other_search_engine_time_period_storage_.GetPeriodSumInTimeRange(
      GetStartOfStalePeriod(), GetEndOfStalePeriod());
}

///////////////////////////////////////////////////////////////////////////////

base::Time SerpMetrics::GetStartOfYesterday() const {
  return base::Time::Now().LocalMidnight() - base::Days(1);
}

base::Time SerpMetrics::GetEndOfYesterday() const {
  return base::Time::Now().LocalMidnight() - base::Milliseconds(1);
}

base::Time SerpMetrics::GetStartOfStalePeriod() const {
  const std::string& last_check_ymd = locale_state_->GetString(kLastCheckYMD);
  if (last_check_ymd.empty()) {
    // If we don't have a last check date, assume the full time period.
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

base::Time SerpMetrics::GetEndOfStalePeriod() const {
  return GetStartOfYesterday() - base::Milliseconds(1);
}

}  // namespace metrics
