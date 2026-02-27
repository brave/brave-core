/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_AGGREGATOR_MOCK_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_AGGREGATOR_MOCK_H_

#include <cstddef>

#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics_aggregator.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace serp_metrics {

class SerpMetricsAggregatorMock final : public SerpMetricsAggregator {
 public:
  SerpMetricsAggregatorMock();

  SerpMetricsAggregatorMock(const SerpMetricsAggregatorMock&) = delete;
  SerpMetricsAggregatorMock& operator=(const SerpMetricsAggregatorMock&) =
      delete;

  ~SerpMetricsAggregatorMock() override;

  MOCK_METHOD(size_t,
              GetSearchCountForYesterday,
              (SerpMetricType type),
              (const, override));

  MOCK_METHOD(size_t, GetSearchCountForStalePeriod, (), (const, override));
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_AGGREGATOR_MOCK_H_
