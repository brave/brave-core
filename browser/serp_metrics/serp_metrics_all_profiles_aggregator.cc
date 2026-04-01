/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_all_profiles_aggregator.h"

#include <numeric>

#include "brave/browser/serp_metrics/serp_metrics_time_period_store_factory.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"

namespace serp_metrics {

SerpMetricsAllProfilesAggregator::SerpMetricsAllProfilesAggregator(
    PrefService* local_state,
    ProfileAttributesStorage& profile_attributes_storage) {
  for (ProfileAttributesEntry* entry :
       profile_attributes_storage.GetAllProfilesAttributes()) {
    const base::FilePath& profile_path = entry->GetPath();
    profile_attributes_serp_metrics_.push_back(std::make_unique<SerpMetrics>(
        local_state, SerpMetricsTimePeriodStoreFactory(
                         profile_path, profile_attributes_storage)));
  }
}

SerpMetricsAllProfilesAggregator::~SerpMetricsAllProfilesAggregator() = default;

size_t SerpMetricsAllProfilesAggregator::GetSearchCountForYesterday(
    SerpMetricType type) const {
  return std::accumulate(
      profile_attributes_serp_metrics_.cbegin(),
      profile_attributes_serp_metrics_.cend(), size_t{0},
      [type](size_t count, const std::unique_ptr<SerpMetrics>& serp_metrics) {
        return count + serp_metrics->GetSearchCountForYesterday(type);
      });
}

size_t SerpMetricsAllProfilesAggregator::GetSearchCountForLastWeek(
    SerpMetricType type) const {
  return std::accumulate(
      profile_attributes_serp_metrics_.cbegin(),
      profile_attributes_serp_metrics_.cend(), size_t{0},
      [type](size_t count, const std::unique_ptr<SerpMetrics>& serp_metrics) {
        return count + serp_metrics->GetSearchCountForLastWeek(type);
      });
}

size_t SerpMetricsAllProfilesAggregator::GetSearchCountForLastMonth(
    SerpMetricType type) const {
  return std::accumulate(
      profile_attributes_serp_metrics_.cbegin(),
      profile_attributes_serp_metrics_.cend(), size_t{0},
      [type](size_t count, const std::unique_ptr<SerpMetrics>& serp_metrics) {
        return count + serp_metrics->GetSearchCountForLastMonth(type);
      });
}

size_t SerpMetricsAllProfilesAggregator::GetSearchCountForStalePeriod() const {
  return std::accumulate(
      profile_attributes_serp_metrics_.cbegin(),
      profile_attributes_serp_metrics_.cend(), size_t{0},
      [](size_t count, const std::unique_ptr<SerpMetrics>& serp_metrics) {
        return count + serp_metrics->GetSearchCountForStalePeriod();
      });
}

size_t SerpMetricsAllProfilesAggregator::GetSearchCountForStaleWeekPeriod()
    const {
  return std::accumulate(
      profile_attributes_serp_metrics_.cbegin(),
      profile_attributes_serp_metrics_.cend(), size_t{0},
      [](size_t count, const std::unique_ptr<SerpMetrics>& serp_metrics) {
        return count + serp_metrics->GetSearchCountForStaleWeekPeriod();
      });
}

size_t SerpMetricsAllProfilesAggregator::GetSearchCountForStaleMonthPeriod()
    const {
  return std::accumulate(
      profile_attributes_serp_metrics_.cbegin(),
      profile_attributes_serp_metrics_.cend(), size_t{0},
      [](size_t count, const std::unique_ptr<SerpMetrics>& serp_metrics) {
        return count + serp_metrics->GetSearchCountForStaleMonthPeriod();
      });
}

size_t SerpMetricsAllProfilesAggregator::GetSearchCountForTesting(  // IN-TEST
    SerpMetricType type) const {
  return std::accumulate(
      profile_attributes_serp_metrics_.cbegin(),
      profile_attributes_serp_metrics_.cend(), size_t{0},
      [type](size_t count, const std::unique_ptr<SerpMetrics>& serp_metrics) {
        return count + serp_metrics->GetSearchCountForTesting(type);  // IN-TEST
      });
}

}  // namespace serp_metrics
