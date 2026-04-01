/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_ALL_PROFILES_AGGREGATOR_MOCK_H_
#define BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_ALL_PROFILES_AGGREGATOR_MOCK_H_

#include <cstddef>

#include "brave/browser/serp_metrics/serp_metrics_all_profiles_aggregator.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "testing/gmock/include/gmock/gmock.h"

class PrefService;
class ProfileAttributesStorage;

namespace serp_metrics {

class SerpMetricsAllProfilesAggregatorMock
    : public SerpMetricsAllProfilesAggregator {
 public:
  SerpMetricsAllProfilesAggregatorMock(
      PrefService* local_state,
      ProfileAttributesStorage& profile_attributes_storage);

  SerpMetricsAllProfilesAggregatorMock(
      const SerpMetricsAllProfilesAggregatorMock&) = delete;
  SerpMetricsAllProfilesAggregatorMock& operator=(
      const SerpMetricsAllProfilesAggregatorMock&) = delete;

  ~SerpMetricsAllProfilesAggregatorMock() override;

  MOCK_METHOD(size_t,
              GetSearchCountForYesterday,
              (SerpMetricType type),
              (const, override));

  MOCK_METHOD(size_t,
              GetSearchCountForLastWeek,
              (SerpMetricType type),
              (const, override));

  MOCK_METHOD(size_t,
              GetSearchCountForLastMonth,
              (SerpMetricType type),
              (const, override));

  MOCK_METHOD(size_t, GetSearchCountForStalePeriod, (), (const, override));
};

}  // namespace serp_metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_ALL_PROFILES_AGGREGATOR_MOCK_H_
