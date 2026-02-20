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

struct TimePeriodStorageInfo {
  SerpMetricType type;
  const char* dict_key;
};

constexpr TimePeriodStorageInfo kTimePeriodStorages[] = {
    {.type = SerpMetricType::kBrave, .dict_key = "brave_search_engine"},
    {.type = SerpMetricType::kGoogle, .dict_key = "google_search_engine"},
    {.type = SerpMetricType::kOther, .dict_key = "other_search_engine"},
};

base::flat_map<SerpMetricType, std::unique_ptr<TimePeriodStorage>>
BuildTimePeriodStorages(PrefService* prefs) {
  base::flat_map<SerpMetricType, std::unique_ptr<TimePeriodStorage>>
      time_period_storages;

  for (const auto& [type, dict_key] : kTimePeriodStorages) {
    time_period_storages.emplace(
        type, std::make_unique<TimePeriodStorage>(
                  prefs, prefs::kSerpMetricsTimePeriodStorage, dict_key,
                  kSerpMetricsTimePeriodInDays.Get(),
                  /*should_offset_dst=*/false));
  }

  return time_period_storages;
}

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
      time_period_storages_(BuildTimePeriodStorages(prefs)) {
  CHECK(local_state_);
  CHECK(base::FeatureList::IsEnabled(serp_metrics::kSerpMetricsFeature));
}

SerpMetrics::~SerpMetrics() = default;

void SerpMetrics::RecordSearch(SerpMetricType type) {
  CHECK(time_period_storages_.contains(type));
  time_period_storages_.at(type)->AddDelta(1);
}

size_t SerpMetrics::GetSearchCountForYesterday(SerpMetricType type) const {
  CHECK(time_period_storages_.contains(type));
  return GetYesterdaySumAfterLastCheckedCutoff(
      *time_period_storages_.at(type), GetStartOfYesterday(),
      GetEndOfYesterday(), GetStartOfStalePeriod());
}

size_t SerpMetrics::GetSearchCountForStalePeriod() const {
  const base::Time start_of_stale_period = GetStartOfStalePeriod();
  const base::Time end_of_stale_period = GetEndOfStalePeriod();

  size_t count = 0;
  for (const auto& [_, time_period_storage] : time_period_storages_) {
    count += time_period_storage->GetPeriodSumInTimeRange(start_of_stale_period,
                                                          end_of_stale_period);
  }
  return count;
}

void SerpMetrics::ClearHistory() {
  for (auto& [_, time_period_storage] : time_period_storages_) {
    time_period_storage->Clear();
  }
}

size_t SerpMetrics::GetSearchCountForTesting(SerpMetricType type) const {
  CHECK(time_period_storages_.contains(type));
  return time_period_storages_.at(type)->GetPeriodSum();
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

  // Return the start of the ping day. The ping associated with `kLastCheckYMD`
  // reports SERP metrics collected through the end of the prior day and does
  // not include any data from `kLastCheckYMD` itself.
  return last_checked_at.LocalMidnight();
}

}  // namespace serp_metrics
