/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_ALL_PROFILES_AGGREGATOR_H_
#define BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_ALL_PROFILES_AGGREGATOR_H_

#include <cstddef>
#include <memory>
#include <vector>

#include "brave/components/serp_metrics/serp_metric_type.h"

class PrefService;
class ProfileAttributesStorage;

namespace serp_metrics {

class SerpMetrics;

// Aggregates SERP metrics across all profiles. Instances should not be cached
// as they may become out of sync with the SERP metrics of loaded profiles.
class SerpMetricsAllProfilesAggregator {
 public:
  SerpMetricsAllProfilesAggregator(
      PrefService* local_state,
      ProfileAttributesStorage& profile_attributes_storage);

  SerpMetricsAllProfilesAggregator(const SerpMetricsAllProfilesAggregator&) =
      delete;
  SerpMetricsAllProfilesAggregator& operator=(
      const SerpMetricsAllProfilesAggregator&) = delete;

  virtual ~SerpMetricsAllProfilesAggregator();

  // Returns yesterday's search count for `type`, summed across all profiles.
  virtual size_t GetSearchCountForYesterday(SerpMetricType type) const;

  // Returns last week's search count for `type`, summed across all profiles.
  virtual size_t GetSearchCountForLastWeek(SerpMetricType type) const;

  // Returns last month's search count for `type`, summed across all profiles.
  virtual size_t GetSearchCountForLastMonth(SerpMetricType type) const;
  // Returns searches older than yesterday but within the retention window,
  // summed across all profiles and engine types.
  virtual size_t GetSearchCountForStalePeriod() const;

  // Returns searches older than last week's start but within the retention
  // window, summed across all profiles and engine types.
  virtual size_t GetSearchCountForStaleWeekPeriod() const;

  // Returns searches older than last month's start but within the retention
  // window, summed across all profiles and engine types.
  virtual size_t GetSearchCountForStaleMonthPeriod() const;

  size_t GetSearchCountForTesting(SerpMetricType type) const;

 private:
  std::vector<std::unique_ptr<SerpMetrics>> profile_attributes_serp_metrics_;
};

}  // namespace serp_metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_ALL_PROFILES_AGGREGATOR_H_
