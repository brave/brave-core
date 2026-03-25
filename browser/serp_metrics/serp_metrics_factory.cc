/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_factory.h"

#include <memory>

#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "brave/components/time_period_storage/time_period_store.h"
#include "brave/components/time_period_storage/time_period_store_factory.h"

namespace serp_metrics {

namespace {

struct TimePeriodStorageInfo {
  SerpMetricType type = SerpMetricType::kUndefined;
  const char* metric_name = nullptr;
};

constexpr TimePeriodStorageInfo kTimePeriodStorages[] = {
    {.type = SerpMetricType::kBrave, .metric_name = "brave_search_engine"},
    {.type = SerpMetricType::kGoogle, .metric_name = "google_search_engine"},
    {.type = SerpMetricType::kOther, .metric_name = "other_search_engine"},
};

TimePeriodStorages BuildTimePeriodStorages(
    const TimePeriodStoreFactory& time_period_store_factory) {
  TimePeriodStorages time_period_storages;
  for (const auto& [type, metric_name] : kTimePeriodStorages) {
    time_period_storages.emplace(
        type, std::make_unique<TimePeriodStorage>(
                  time_period_store_factory.Build(metric_name),
                  kSerpMetricsTimePeriodInDays.Get(),
                  /*should_offset_dst=*/false));
  }

  return time_period_storages;
}

}  // namespace

std::unique_ptr<SerpMetrics> CreateSerpMetrics(
    PrefService* local_state,
    const TimePeriodStoreFactory& time_period_store_factory) {
  return std::make_unique<SerpMetrics>(
      local_state, BuildTimePeriodStorages(time_period_store_factory));
}

}  // namespace serp_metrics
