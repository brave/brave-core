/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TEST_SERP_METRICS_TEST_BASE_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TEST_SERP_METRICS_TEST_BASE_H_

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

// Base test fixture for `SerpMetrics` unit tests. Shared by the yesterday,
// weekly, and monthly test suites. `SetUp` registers prefs, initialises the
// feature, and constructs `serp_metrics_`.
class SerpMetricsTestBase : public testing::Test {
 public:
  SerpMetricsTestBase();
  ~SerpMetricsTestBase() override;

  void SetUp() override;

  // Advances the clock beyond the retention period, causing expired metrics to
  // be dropped from `TimePeriodStorage`.
  void AdvanceClockByRetentionPeriod();

 protected:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<SerpMetrics> serp_metrics_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TEST_SERP_METRICS_TEST_BASE_H_
