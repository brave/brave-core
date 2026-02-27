/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_factory.h"

#include <memory>
#include <string_view>

#include "base/files/file_path.h"
#include "brave/browser/serp_metrics/serp_metrics_time_period_store.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"

namespace serp_metrics {

namespace {

struct TimePeriodStorageInfo {
  SerpMetricType type = SerpMetricType::kUndefined;
  std::string_view metric_name;
};

constexpr TimePeriodStorageInfo kTimePeriodStorages[] = {
    {.type = SerpMetricType::kBrave, .metric_name = "brave_search_engine"},
    {.type = SerpMetricType::kGoogle, .metric_name = "google_search_engine"},
    {.type = SerpMetricType::kOther, .metric_name = "other_search_engine"},
    {.type = SerpMetricType::kYouTube, .metric_name = "youtube_search_engine"}};

TimePeriodStorages BuildTimePeriodStorages(
    const base::FilePath& profile_path,
    ProfileAttributesStorage& profile_attributes_storage) {
  TimePeriodStorages time_period_storages;
  for (const auto& [type, metric_name] : kTimePeriodStorages) {
    time_period_storages.emplace(
        type, std::make_unique<TimePeriodStorage>(
                  std::make_unique<SerpMetricsTimePeriodStore>(
                      profile_path, profile_attributes_storage,
                      std::string(metric_name)),
                  kSerpMetricsTimePeriodInDays.Get(),
                  /*should_offset_dst=*/false));
  }

  return time_period_storages;
}

}  // namespace

std::unique_ptr<SerpMetrics> CreateSerpMetrics(
    PrefService* local_state,
    const base::FilePath& profile_path,
    ProfileAttributesStorage& profile_attributes_storage) {
  return std::make_unique<SerpMetrics>(
      local_state,
      BuildTimePeriodStorages(profile_path, profile_attributes_storage));
}

}  // namespace serp_metrics
