/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/feature_list.h"
#include "base/time/time.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_storage.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store_factory.h"
#include "components/prefs/pref_service.h"

namespace serp_metrics {

namespace {

struct TimePeriodStorageInfo {
  SerpMetricType serp_metric_type = SerpMetricType::kUndefined;
  std::string_view pref_key;
};

constexpr TimePeriodStorageInfo kTimePeriodStorages[] = {
    {.serp_metric_type = SerpMetricType::kBrave,
     .pref_key = "brave_search_engine"},
    {.serp_metric_type = SerpMetricType::kGoogle,
     .pref_key = "google_search_engine"},
    {.serp_metric_type = SerpMetricType::kOther,
     .pref_key = "other_search_engine"},
};

base::flat_map<SerpMetricType, std::unique_ptr<SerpMetricsTimePeriodStorage>>
BuildTimePeriodStorages(
    const SerpMetricsTimePeriodStoreFactory& time_period_store_factory) {
  base::flat_map<SerpMetricType, std::unique_ptr<SerpMetricsTimePeriodStorage>>
      time_period_storages;
  for (const auto& [type, pref_key] : kTimePeriodStorages) {
    std::unique_ptr<SerpMetricsTimePeriodStore> time_period_store =
        time_period_store_factory.Build(pref_key);
    time_period_storages.emplace(
        type,
        std::make_unique<SerpMetricsTimePeriodStorage>(
            std::move(time_period_store), kSerpMetricsTimePeriodInDays.Get()));
  }

  return time_period_storages;
}

// Returns the start of yesterday in UTC (midnight beginning the previous UTC
// calendar day). UTC days are always exactly 24 hours, so no DST correction
// is needed.
base::Time GetStartOfYesterday(base::Time now) {
  return now.UTCMidnight() - base::Days(1);
}

// Returns the end of yesterday in UTC, defined as the final millisecond
// before today's UTC midnight. Subtracting one millisecond ensures the
// yesterday time range is inclusive of all events on that day without spilling
// into today.
base::Time GetEndOfYesterday(base::Time now) {
  return now.UTCMidnight() - base::Milliseconds(1);
}

// Returns the end of the stale period in UTC, defined as the final
// millisecond before the start of yesterday. This establishes a clear boundary
// between stale metrics and yesterday's metrics without overlap.
base::Time GetEndOfStalePeriod(base::Time now) {
  return GetStartOfYesterday(now) - base::Milliseconds(1);
}

// Returns the UTC midnight of the Monday that starts the current ISO week.
base::Time GetStartOfThisWeek(base::Time now) {
  base::Time::Exploded exploded;
  now.UTCExplode(&exploded);
  const int days_since_monday =
      (exploded.day_of_week == 0) ? 6 : exploded.day_of_week - 1;
  return (now - base::Days(days_since_monday)).UTCMidnight();
}

// Returns the UTC midnight of the Monday that started the previous ISO week.
base::Time GetStartOfLastWeek(base::Time now) {
  return GetStartOfThisWeek(now) - base::Days(7);
}

// Returns the final millisecond of the previous ISO week (Sunday 23:59:59.999
// UTC).
base::Time GetEndOfLastWeek(base::Time now) {
  return GetStartOfThisWeek(now) - base::Milliseconds(1);
}

// Returns UTC midnight on the first day of the current calendar month.
base::Time GetStartOfThisMonth(base::Time now) {
  base::Time::Exploded exploded;
  now.UTCExplode(&exploded);
  return (now - base::Days(exploded.day_of_month - 1)).UTCMidnight();
}

// Returns UTC midnight on the first day of the previous calendar month.
base::Time GetStartOfLastMonth(base::Time now) {
  return GetStartOfThisMonth(GetStartOfThisMonth(now) - base::Milliseconds(1));
}

// Returns the final millisecond of the previous calendar month
// (last day 23:59:59.999 UTC).
base::Time GetEndOfLastMonth(base::Time now) {
  return GetStartOfThisMonth(now) - base::Milliseconds(1);
}

// Returns the sum of metrics recorded during yesterday (UTC) that have
// not already been reported. The later of the start of yesterday and the start
// of the stale period is used as the cutoff to avoid double-counting previously
// reported metrics. If the resulting time range does not include any portion of
// yesterday, the function returns 0.
size_t GetYesterdaySumAfterLastCheckedCutoff(
    const SerpMetricsTimePeriodStorage& time_period_storage,
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

  return time_period_storage.GetCountForTimeRange(start_time, end_of_yesterday);
}

}  // namespace

SerpMetrics::SerpMetrics(
    PrefService* local_state,
    const SerpMetricsTimePeriodStoreFactory& time_period_store_factory)
    : local_state_(*local_state),
      time_period_storages_(
          BuildTimePeriodStorages(time_period_store_factory)) {
  CHECK(base::FeatureList::IsEnabled(serp_metrics::kSerpMetricsFeature));
}

SerpMetrics::~SerpMetrics() = default;

void SerpMetrics::RecordSearch(SerpMetricType type) {
  CHECK_NE(SerpMetricType::kUndefined, type);

  const auto iter = time_period_storages_.find(type);
  CHECK(iter != time_period_storages_.cend());
  SerpMetricsTimePeriodStorage& storage = *iter->second;
  storage.AddCount(1);
}

size_t SerpMetrics::GetSearchCountForYesterday(SerpMetricType type) const {
  CHECK_NE(SerpMetricType::kUndefined, type);

  const auto iter = time_period_storages_.find(type);
  CHECK(iter != time_period_storages_.cend());
  const SerpMetricsTimePeriodStorage& storage = *iter->second;
  const base::Time now = base::Time::Now();
  return GetYesterdaySumAfterLastCheckedCutoff(
      storage, GetStartOfYesterday(now), GetEndOfYesterday(now),
      GetStartOfStaleDayPeriod());
}

size_t SerpMetrics::GetSearchCountForLastWeek(SerpMetricType type) const {
  CHECK_NE(SerpMetricType::kUndefined, type);
  CHECK(time_period_storages_.contains(type));
  const base::Time now = base::Time::Now();
  return time_period_storages_.at(type)->GetCountForTimeRange(
      GetStartOfLastWeek(now), GetEndOfLastWeek(now));
}

size_t SerpMetrics::GetSearchCountForLastMonth(SerpMetricType type) const {
  CHECK_NE(SerpMetricType::kUndefined, type);
  CHECK(time_period_storages_.contains(type));
  const base::Time now = base::Time::Now();
  return time_period_storages_.at(type)->GetCountForTimeRange(
      GetStartOfLastMonth(now), GetEndOfLastMonth(now));
}

size_t SerpMetrics::GetSearchCountForStalePeriod() const {
  const base::Time now = base::Time::Now();
  const base::Time start_of_stale_period = GetStartOfStaleDayPeriod();
  const base::Time end_of_stale_period = GetEndOfStalePeriod(now);

  // `GetStartOfStaleDayPeriod` returns an empty time when no daily ping has
  // been sent yet, which causes `GetCountForTimeRange` to sum the full
  // retention window. This is intentional since all stored data is stale.
  size_t count = 0;
  for (const auto& [_, time_period_storage] : time_period_storages_) {
    count += time_period_storage->GetCountForTimeRange(start_of_stale_period,
                                                       end_of_stale_period);
  }
  return count;
}

size_t SerpMetrics::GetSearchCountForStaleWeekPeriod() const {
  const base::Time now = base::Time::Now();
  const base::Time start_of_stale_week_period = GetStartOfStaleWeekPeriod();
  const base::Time end_of_stale_week_period =
      GetStartOfLastWeek(now) - base::Milliseconds(1);

  // `GetStartOfStaleWeekPeriod` returns an empty time when no weekly ping has
  // been sent yet, which causes `GetCountForTimeRange` to sum the full
  // retention window. This is intentional since all stored data is stale.
  size_t count = 0;
  for (const auto& [_, time_period_storage] : time_period_storages_) {
    count += time_period_storage->GetCountForTimeRange(
        start_of_stale_week_period, end_of_stale_week_period);
  }
  return count;
}

size_t SerpMetrics::GetSearchCountForStaleMonthPeriod() const {
  const base::Time now = base::Time::Now();
  const base::Time start_of_stale_month_period = GetStartOfStaleMonthPeriod();
  const base::Time end_of_stale_month_period =
      GetStartOfLastMonth(now) - base::Milliseconds(1);

  // `GetStartOfStaleMonthPeriod` returns an empty time when no monthly ping has
  // been sent yet, which causes `GetCountForTimeRange` to sum the full
  // retention window. This is intentional since all stored data is stale.
  size_t count = 0;
  for (const auto& [_, time_period_storage] : time_period_storages_) {
    count += time_period_storage->GetCountForTimeRange(
        start_of_stale_month_period, end_of_stale_month_period);
  }
  return count;
}

void SerpMetrics::ClearHistory() {
  for (auto& [_, time_period_storage] : time_period_storages_) {
    time_period_storage->Clear();
  }
}

size_t SerpMetrics::GetSearchCountForTesting(SerpMetricType type) const {
  CHECK_NE(SerpMetricType::kUndefined, type);

  const auto iter = time_period_storages_.find(type);
  CHECK(iter != time_period_storages_.cend());
  const SerpMetricsTimePeriodStorage& storage = *iter->second;
  return storage.GetCount();
}

///////////////////////////////////////////////////////////////////////////////

base::Time SerpMetrics::GetStartOfStaleDayPeriod() const {
  // `kLastReportedAt` records the UTC timestamp of the last successful
  // daily ping. A null time means no ping has been sent, so the full retention
  // window is stale.
  const base::Time last_daily_reported_at =
      local_state_->GetTime(prefs::kLastReportedAt);
  if (last_daily_reported_at.is_null()) {
    return {};
  }
  return last_daily_reported_at.UTCMidnight();
}

base::Time SerpMetrics::GetStartOfStaleWeekPeriod() const {
  // `kLastWeeklyReportedAt` records the UTC timestamp of the last successful
  // weekly ping. A null time means no weekly ping has been sent, so the full
  // retention window is stale.
  const base::Time last_weekly_reported_at =
      local_state_->GetTime(prefs::kLastWeeklyReportedAt);
  if (last_weekly_reported_at.is_null()) {
    return {};
  }
  return GetStartOfThisWeek(last_weekly_reported_at);
}

base::Time SerpMetrics::GetStartOfStaleMonthPeriod() const {
  // `kLastMonthlyReportedAt` records the UTC timestamp of the last successful
  // monthly ping. A null time means no monthly ping has been sent, so the full
  // retention window is stale.
  const base::Time last_monthly_reported_at =
      local_state_->GetTime(prefs::kLastMonthlyReportedAt);
  if (last_monthly_reported_at.is_null()) {
    return {};
  }
  return GetStartOfThisMonth(last_monthly_reported_at);
}

}  // namespace serp_metrics
