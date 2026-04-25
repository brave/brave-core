/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/split_view_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class SplitViewMetricsTest : public testing::Test {
 public:
  SplitViewMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    SplitViewMetrics::RegisterPrefs(pref_service_.registry());
    metrics_ = std::make_unique<SplitViewMetrics>(&pref_service_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<SplitViewMetrics> metrics_;
  base::HistogramTester histogram_tester_;
};

TEST_F(SplitViewMetricsTest, MonthlyUsage) {
  // Initial report should show 0 usage
  histogram_tester_.ExpectUniqueSample(kSplitViewUsageHistogramName, 0, 1);

  // Report usage once
  metrics_->ReportSplitViewUsage();

  // Should now show 1 usage (bucket 1 = 1-5)
  histogram_tester_.ExpectBucketCount(kSplitViewUsageHistogramName, 1, 1);
  histogram_tester_.ExpectTotalCount(kSplitViewUsageHistogramName, 2);

  // Report usage 4 more times (total 5)
  for (int i = 0; i < 4; ++i) {
    metrics_->ReportSplitViewUsage();
  }

  // Should still be in bucket 1 (1-5 uses)
  histogram_tester_.ExpectBucketCount(kSplitViewUsageHistogramName, 1, 5);
  histogram_tester_.ExpectTotalCount(kSplitViewUsageHistogramName, 6);

  // Fast forward a day to trigger the daily report
  task_environment_.FastForwardBy(base::Days(1));

  // Should report the same 5 usages again
  histogram_tester_.ExpectBucketCount(kSplitViewUsageHistogramName, 1, 6);
  histogram_tester_.ExpectTotalCount(kSplitViewUsageHistogramName, 7);

  // Report usage 6 more times (total 11)
  for (int i = 0; i < 6; ++i) {
    metrics_->ReportSplitViewUsage();
  }

  // Should now be in bucket 2 (6-11 uses)
  histogram_tester_.ExpectBucketCount(kSplitViewUsageHistogramName, 2, 6);
  histogram_tester_.ExpectTotalCount(kSplitViewUsageHistogramName, 13);

  // Fast forward a month to ensure monthly data is still reported
  task_environment_.FastForwardBy(base::Days(30));

  // Should still report the same 22 usages in bucket 4
  histogram_tester_.ExpectBucketCount(kSplitViewUsageHistogramName, 2, 35);
  histogram_tester_.ExpectBucketCount(kSplitViewUsageHistogramName, 0, 2);
  histogram_tester_.ExpectTotalCount(kSplitViewUsageHistogramName, 43);
}

}  // namespace misc_metrics
