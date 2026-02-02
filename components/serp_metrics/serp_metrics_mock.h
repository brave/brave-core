/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_MOCK_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_MOCK_H_

#include <cstddef>

#include "brave/components/serp_metrics/serp_metrics.h"
#include "testing/gmock/include/gmock/gmock.h"

class PrefService;

namespace serp_metrics {

class SerpMetricsMock final : public SerpMetrics {
 public:
  SerpMetricsMock(PrefService* local_state, PrefService* prefs);

  SerpMetricsMock(const SerpMetricsMock&) = delete;
  SerpMetricsMock& operator=(const SerpMetricsMock&) = delete;

  ~SerpMetricsMock() override;

  MOCK_METHOD(void, RecordBraveSearch, (), ());
  MOCK_METHOD(size_t, GetBraveSearchCountForYesterday, (), (const));

  MOCK_METHOD(void, RecordGoogleSearch, (), ());
  MOCK_METHOD(size_t, GetGoogleSearchCountForYesterday, (), (const));

  MOCK_METHOD(void, RecordOtherSearch, (), ());
  MOCK_METHOD(size_t, GetOtherSearchCountForYesterday, (), (const));

  MOCK_METHOD(size_t, GetSearchCountForStalePeriod, (), (const));
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_MOCK_H_
