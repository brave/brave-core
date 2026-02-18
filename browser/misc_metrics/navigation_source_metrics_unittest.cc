/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/navigation_source_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class NavigationSourceMetricsUnitTest : public testing::Test {
 public:
  NavigationSourceMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    NavigationSourceMetrics::RegisterPrefs(local_state_.registry());
    metrics_ = std::make_unique<NavigationSourceMetrics>(&local_state_);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<NavigationSourceMetrics> metrics_;
};

TEST_F(NavigationSourceMetricsUnitTest, NoReportBeforeFrameExpires) {
  for (int i = 0; i < 10; i++) {
    metrics_->IncrementPagesLoadedCount();
  }
  metrics_->RecordBookmarkNavigation();

  // Advance only 12 hours - should not trigger a report.
  task_environment_.FastForwardBy(base::Hours(12));
  metrics_->ReportNavigationSources();

  histogram_tester_.ExpectTotalCount(
      kNavSourceBookmarksSourcePercentHistogramName, 0);
}

TEST_F(NavigationSourceMetricsUnitTest, NoReportWhenTotalIsZero) {
  // Advance 24 hours with no navigations.
  task_environment_.FastForwardBy(base::Days(1));
  metrics_->ReportNavigationSources();

  histogram_tester_.ExpectTotalCount(
      kNavSourceBookmarksSourcePercentHistogramName, 0);
  histogram_tester_.ExpectTotalCount(
      kNavSourceDirectURLSourcePercentHistogramName, 0);
}

TEST_F(NavigationSourceMetricsUnitTest, BookmarkBarNavigation) {
  // 10 total navigations, 5 from bookmark bar.
  for (int i = 0; i < 10; i++) {
    metrics_->IncrementPagesLoadedCount();
  }
  for (int i = 0; i < 5; i++) {
    metrics_->RecordBookmarkNavigation();
  }

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->ReportNavigationSources();

  // 5/10 = 50% -> bucket 3 (21-80%)
  histogram_tester_.ExpectUniqueSample(
      kNavSourceBookmarksSourcePercentHistogramName, 3, 1);
  histogram_tester_.ExpectTotalCount(
      kNavSourceDirectURLSourcePercentHistogramName, 0);
}

TEST_F(NavigationSourceMetricsUnitTest, OmniboxBookmarkNavigation) {
  for (int i = 0; i < 10; i++) {
    metrics_->IncrementPagesLoadedCount();
  }
  for (int i = 0; i < 2; i++) {
    metrics_->RecordBookmarkNavigation();
  }

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->ReportNavigationSources();

  // 2/10 = 20% -> bucket 2 (6-20%)
  histogram_tester_.ExpectUniqueSample(
      kNavSourceBookmarksSourcePercentHistogramName, 2, 1);
}

TEST_F(NavigationSourceMetricsUnitTest, DirectURLNavigation) {
  for (int i = 0; i < 100; i++) {
    metrics_->IncrementPagesLoadedCount();
  }
  for (int i = 0; i < 90; i++) {
    metrics_->RecordDirectNavigation();
  }

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->ReportNavigationSources();

  // 90/100 = 90% -> bucket 4 (81-95%)
  histogram_tester_.ExpectUniqueSample(
      kNavSourceDirectURLSourcePercentHistogramName, 4, 1);
}

TEST_F(NavigationSourceMetricsUnitTest, HistoryNavigation) {
  for (int i = 0; i < 100; i++) {
    metrics_->IncrementPagesLoadedCount();
  }
  for (int i = 0; i < 15; i++) {
    metrics_->RecordHistoryNavigation();
  }

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->ReportNavigationSources();

  // 15/100 = 15% -> bucket 2 (6-20%)
  histogram_tester_.ExpectUniqueSample(
      kNavSourceHistorySourcePercentHistogramName, 2, 1);
}

TEST_F(NavigationSourceMetricsUnitTest, CustomTopSiteNavigation) {
  for (int i = 0; i < 20; i++) {
    metrics_->IncrementPagesLoadedCount();
  }
  for (int i = 0; i < 4; i++) {
    metrics_->RecordTopSiteNavigation(/*is_custom=*/true);
  }

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->ReportNavigationSources();

  // 4/20 = 20% -> bucket 2 (6-20%)
  histogram_tester_.ExpectUniqueSample(
      kNavSourceCustomTopSitesSourcePercentHistogramName, 2, 1);
  histogram_tester_.ExpectTotalCount(
      kNavSourceFrequentTopSitesSourcePercentHistogramName, 0);
}

TEST_F(NavigationSourceMetricsUnitTest, FrequentTopSiteNavigation) {
  for (int i = 0; i < 20; i++) {
    metrics_->IncrementPagesLoadedCount();
  }
  for (int i = 0; i < 4; i++) {
    metrics_->RecordTopSiteNavigation(/*is_custom=*/false);
  }

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->ReportNavigationSources();

  // 4/20 = 20% -> bucket 2 (6-20%)
  histogram_tester_.ExpectUniqueSample(
      kNavSourceFrequentTopSitesSourcePercentHistogramName, 2, 1);
  histogram_tester_.ExpectTotalCount(
      kNavSourceCustomTopSitesSourcePercentHistogramName, 0);
}

TEST_F(NavigationSourceMetricsUnitTest, CountsClearedAfterReport) {
  for (int i = 0; i < 10; i++) {
    metrics_->IncrementPagesLoadedCount();
  }
  metrics_->RecordBookmarkNavigation();

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->ReportNavigationSources();

  histogram_tester_.ExpectTotalCount(
      kNavSourceBookmarksSourcePercentHistogramName, 1);

  // Record nothing in the next window.
  task_environment_.FastForwardBy(base::Days(1));
  metrics_->ReportNavigationSources();

  // No new sample should be recorded (total is 0).
  histogram_tester_.ExpectTotalCount(
      kNavSourceBookmarksSourcePercentHistogramName, 1);
}

TEST_F(NavigationSourceMetricsUnitTest, ZeroPercentSourceNotReported) {
  // 10 total, 0 bookmarks.
  for (int i = 0; i < 10; i++) {
    metrics_->IncrementPagesLoadedCount();
  }

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->ReportNavigationSources();

  histogram_tester_.ExpectTotalCount(
      kNavSourceBookmarksSourcePercentHistogramName, 0);
  histogram_tester_.ExpectTotalCount(
      kNavSourceHistorySourcePercentHistogramName, 0);
  histogram_tester_.ExpectTotalCount(
      kNavSourceCustomTopSitesSourcePercentHistogramName, 0);
}

TEST_F(NavigationSourceMetricsUnitTest,
       SmallPercentageReportsAtLeastOneBucket) {
  // 100 total, 1 bookmark -> 1% -> bucket 1 (1-5%)
  for (int i = 0; i < 100; i++) {
    metrics_->IncrementPagesLoadedCount();
  }
  metrics_->RecordBookmarkNavigation();

  task_environment_.FastForwardBy(base::Days(1));
  metrics_->ReportNavigationSources();

  // std::max(1, 1*100/100) = 1% -> bucket 1 (1-5%)
  histogram_tester_.ExpectUniqueSample(
      kNavSourceBookmarksSourcePercentHistogramName, 1, 1);
}

}  // namespace misc_metrics
