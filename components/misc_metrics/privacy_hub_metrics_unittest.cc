/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/misc_metrics/privacy_hub_metrics.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class PrivacyHubMetricsUnitTest : public testing::Test {
 public:
  PrivacyHubMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    misc_metrics::PrivacyHubMetrics::RegisterPrefs(local_state_.registry());
    privacy_hub_metrics_ = std::make_unique<PrivacyHubMetrics>(&local_state_);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<PrivacyHubMetrics> privacy_hub_metrics_;
};

TEST_F(PrivacyHubMetricsUnitTest, Views) {
  histogram_tester_.ExpectTotalCount(kViewsMonthlyHistogramName, 0);

  for (size_t i = 0; i < 4; i++) {
    privacy_hub_metrics_->RecordView();
  }

  histogram_tester_.ExpectBucketCount(kViewsMonthlyHistogramName, 0, 1);
  histogram_tester_.ExpectBucketCount(kViewsMonthlyHistogramName, 1, 3);

  task_environment_.FastForwardBy(base::Days(30));
  histogram_tester_.ExpectBucketCount(kViewsMonthlyHistogramName, 1, 32);

  task_environment_.FastForwardBy(base::Days(10));
  histogram_tester_.ExpectBucketCount(kViewsMonthlyHistogramName, 1, 32);
}

}  // namespace misc_metrics
