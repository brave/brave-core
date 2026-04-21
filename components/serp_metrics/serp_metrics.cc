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
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "brave/components/time_period_storage/time_period_store.h"
#include "brave/components/time_period_storage/time_period_store_factory.h"

namespace serp_metrics {

namespace {

struct TimePeriodStorageInfo {
  SerpMetricType serp_metric_type = SerpMetricType::kUndefined;
  const char* metric_name = nullptr;
};

constexpr TimePeriodStorageInfo kTimePeriodStorages[] = {
    {.serp_metric_type = SerpMetricType::kBrave,
     .metric_name = "brave_search_engine"},
    {.serp_metric_type = SerpMetricType::kGoogle,
     .metric_name = "google_search_engine"},
    {.serp_metric_type = SerpMetricType::kOther,
     .metric_name = "other_search_engine"},
};

base::flat_map<SerpMetricType, std::unique_ptr<TimePeriodStorage>>
BuildTimePeriodStorages(const TimePeriodStoreFactory& time_period_store_factory,
                        bool report_in_utc) {
  base::flat_map<SerpMetricType, std::unique_ptr<TimePeriodStorage>>
      time_period_storages;
  for (const auto& [type, metric_name] : kTimePeriodStorages) {
    std::unique_ptr<TimePeriodStore> time_period_store =
        time_period_store_factory.Build(metric_name);
    time_period_storages.emplace(
        type, std::make_unique<TimePeriodStorage>(
                  std::move(time_period_store),
                  kSerpMetricsTimePeriodInDays.Get(), report_in_utc,
                  /*should_offset_dst=*/false));
  }

  return time_period_storages;
}

// Returns the sum of metrics recorded during yesterday that have not already
// been reported. The later of the start of yesterday and the start of the
// stale period is used as the cutoff to avoid double-counting previously
// reported metrics. If the resulting time range does not include any portion
// of yesterday, the function returns 0.
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

SerpMetrics::SerpMetrics(
    PrefService* local_state,
    const TimePeriodStoreFactory& time_period_store_factory,
    bool report_in_utc)
    : day_boundary_calculator_(local_state, report_in_utc),
      time_period_storages_(
          BuildTimePeriodStorages(time_period_store_factory, report_in_utc)) {
  CHECK(base::FeatureList::IsEnabled(serp_metrics::kSerpMetricsFeature));
}

SerpMetrics::~SerpMetrics() = default;

void SerpMetrics::RecordSearch(SerpMetricType type) {
  CHECK_NE(SerpMetricType::kUndefined, type);
  CHECK(time_period_storages_.contains(type));
  time_period_storages_.at(type)->AddDelta(1);
}

size_t SerpMetrics::GetSearchCountForYesterday(SerpMetricType type) const {
  CHECK_NE(SerpMetricType::kUndefined, type);
  CHECK(time_period_storages_.contains(type));
  const base::Time now = base::Time::Now();
  return GetYesterdaySumAfterLastCheckedCutoff(
      *time_period_storages_.at(type),
      day_boundary_calculator_.GetStartOfYesterday(now),
      day_boundary_calculator_.GetEndOfYesterday(now),
      day_boundary_calculator_.GetStartOfStalePeriod());
}

size_t SerpMetrics::GetSearchCountForStalePeriod() const {
  const base::Time now = base::Time::Now();
  const base::Time start_of_stale_period =
      day_boundary_calculator_.GetStartOfStalePeriod();
  const base::Time end_of_stale_period =
      day_boundary_calculator_.GetEndOfStalePeriod(now);

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
  CHECK_NE(SerpMetricType::kUndefined, type);
  CHECK(time_period_storages_.contains(type));
  return time_period_storages_.at(type)->GetPeriodSum();
}

}  // namespace serp_metrics
