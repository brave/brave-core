/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/misc_metrics/tab_metrics.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class TabMetricsUnitTest : public testing::Test {
 public:
  TabMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    misc_metrics::TabMetrics::RegisterPrefs(local_state_.registry());
    tab_metrics_ = std::make_unique<TabMetrics>(&local_state_);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<TabMetrics> tab_metrics_;
};

TEST_F(TabMetricsUnitTest, NewTabMethods) {
  histogram_tester_.ExpectTotalCount(kNewTabMethodsHistogramName, 0);

  for (size_t i = 0; i < 6; i++) {
    tab_metrics_->RecordAppMenuNewTab();
  }

  histogram_tester_.ExpectBucketCount(kNewTabMethodsHistogramName, 0, 6);

  for (size_t i = 0; i < 2; i++) {
    tab_metrics_->RecordTabSwitcherNewTab();
  }

  histogram_tester_.ExpectBucketCount(kNewTabMethodsHistogramName, 0, 8);

  tab_metrics_->RecordTabSwitcherNewTab();

  histogram_tester_.ExpectBucketCount(kNewTabMethodsHistogramName, 1, 1);

  for (size_t i = 0; i < 4; i++) {
    tab_metrics_->RecordTabSwitcherNewTab();
  }

  histogram_tester_.ExpectBucketCount(kNewTabMethodsHistogramName, 2, 1);

  for (size_t i = 0; i < 12; i++) {
    tab_metrics_->RecordTabSwitcherNewTab();
  }

  histogram_tester_.ExpectBucketCount(kNewTabMethodsHistogramName, 3, 1);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectBucketCount(kNewTabMethodsHistogramName, 3, 7);

  task_environment_.FastForwardBy(base::Days(3));

  histogram_tester_.ExpectBucketCount(kNewTabMethodsHistogramName, 3, 7);
}

TEST_F(TabMetricsUnitTest, LocationNewEntries) {
  histogram_tester_.ExpectTotalCount(kLocationNewEntriesHistogramName, 0);

  for (size_t i = 0; i < 6; i++) {
    tab_metrics_->RecordLocationBarChange(false);
  }

  histogram_tester_.ExpectBucketCount(kLocationNewEntriesHistogramName, 0, 6);

  for (size_t i = 0; i < 2; i++) {
    tab_metrics_->RecordLocationBarChange(true);
  }

  histogram_tester_.ExpectBucketCount(kLocationNewEntriesHistogramName, 0, 8);

  tab_metrics_->RecordLocationBarChange(true);

  histogram_tester_.ExpectBucketCount(kLocationNewEntriesHistogramName, 1, 1);

  for (size_t i = 0; i < 4; i++) {
    tab_metrics_->RecordLocationBarChange(true);
  }

  histogram_tester_.ExpectBucketCount(kLocationNewEntriesHistogramName, 2, 1);

  for (size_t i = 0; i < 12; i++) {
    tab_metrics_->RecordLocationBarChange(true);
  }

  histogram_tester_.ExpectBucketCount(kLocationNewEntriesHistogramName, 3, 1);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectBucketCount(kLocationNewEntriesHistogramName, 3, 7);

  task_environment_.FastForwardBy(base::Days(3));

  histogram_tester_.ExpectBucketCount(kLocationNewEntriesHistogramName, 3, 7);
}

}  // namespace misc_metrics
