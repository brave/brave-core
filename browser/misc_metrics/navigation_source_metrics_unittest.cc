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
  void FastForwardAndReport(base::TimeDelta delta) {
    task_environment_.FastForwardBy(delta);
    metrics_->ReportNavigationSources();
  }

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
  FastForwardAndReport(base::Hours(12));

  histogram_tester_.ExpectTotalCount(
      kNavSourceBookmarksSourcePercentHistogramName, 0);
}

TEST_F(NavigationSourceMetricsUnitTest, NoReportWhenTotalIsZero) {
  // Advance 24 hours with no navigations.
  FastForwardAndReport(base::Days(1));

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

  FastForwardAndReport(base::Days(1));

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

  FastForwardAndReport(base::Days(1));

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

  FastForwardAndReport(base::Days(1));

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

  FastForwardAndReport(base::Days(1));

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

  FastForwardAndReport(base::Days(1));

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

  FastForwardAndReport(base::Days(1));

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

  FastForwardAndReport(base::Days(1));

  histogram_tester_.ExpectTotalCount(
      kNavSourceBookmarksSourcePercentHistogramName, 1);

  // Record nothing in the next window.
  FastForwardAndReport(base::Days(1));

  // No new sample should be recorded (total is 0).
  histogram_tester_.ExpectTotalCount(
      kNavSourceBookmarksSourcePercentHistogramName, 1);
}

TEST_F(NavigationSourceMetricsUnitTest, ZeroPercentSourceNotReported) {
  // 10 total, 0 bookmarks.
  for (int i = 0; i < 10; i++) {
    metrics_->IncrementPagesLoadedCount();
  }

  FastForwardAndReport(base::Days(1));

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

  FastForwardAndReport(base::Days(1));

  // std::max(1, 1*100/100) = 1% -> bucket 1 (1-5%)
  histogram_tester_.ExpectUniqueSample(
      kNavSourceBookmarksSourcePercentHistogramName, 1, 1);
}

TEST_F(NavigationSourceMetricsUnitTest, ExternalNavigation) {
  // 20 total, 6 external -> 30% -> bucket 3 (21-80%)
  for (int i = 0; i < 20; i++) {
    metrics_->IncrementPagesLoadedCount();
  }
  for (int i = 0; i < 6; i++) {
    metrics_->RecordExternalNavigation();
  }

  FastForwardAndReport(base::Days(1));

  histogram_tester_.ExpectUniqueSample(
      kNavSourceExternalSourcePercentHistogramName, 3, 1);
}

TEST_F(NavigationSourceMetricsUnitTest, ExternalNavigationZeroNotReported) {
  // 10 total, 0 external.
  for (int i = 0; i < 10; i++) {
    metrics_->IncrementPagesLoadedCount();
  }

  FastForwardAndReport(base::Days(1));

  histogram_tester_.ExpectTotalCount(
      kNavSourceExternalSourcePercentHistogramName, 0);
}

TEST_F(NavigationSourceMetricsUnitTest, NavigatedInPreviousFrameReported) {
  for (int i = 0; i < 5; i++) {
    metrics_->IncrementPagesLoadedCount();
  }

  // Should not report before the frame expires.
  FastForwardAndReport(base::Hours(12));
  histogram_tester_.ExpectTotalCount(kNavSourceNavigatedHistogramName, 0);

  // Should report once the full day has elapsed.
  FastForwardAndReport(base::Hours(12));
  histogram_tester_.ExpectUniqueSample(kNavSourceNavigatedHistogramName, 1, 1);
}

TEST_F(NavigationSourceMetricsUnitTest,
       NavigatedInPreviousFrameNotReportedWhenNoNavigations) {
  // No navigations recorded.
  FastForwardAndReport(base::Days(1));

  histogram_tester_.ExpectTotalCount(kNavSourceNavigatedHistogramName, 0);
}

}  // namespace misc_metrics
