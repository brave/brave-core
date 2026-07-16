/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/browser/backup_results_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_search {

class BackupResultsMetricsTest : public ::testing::Test {
 protected:
  BackupResultsMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    BackupResultsMetrics::RegisterPrefs(prefs_.registry());
    CreateMetrics();
  }

  void CreateMetrics() {
    backup_results_metrics_ = std::make_unique<BackupResultsMetrics>(&prefs_);
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<BackupResultsMetrics> backup_results_metrics_;
  base::HistogramTester histogram_tester_;
};

TEST_F(BackupResultsMetricsTest, ReportsZeroPercent) {
  backup_results_metrics_->RecordQuery(false);
  backup_results_metrics_->RecordQuery(false);
  backup_results_metrics_->RecordQuery(false);

  // No report should be recorded until a full 24 hour frame has elapsed.
  histogram_tester_.ExpectTotalCount(kBackupResultsFailuresHistogramName, 0);
  task_environment_.FastForwardBy(base::Hours(23));
  histogram_tester_.ExpectTotalCount(kBackupResultsFailuresHistogramName, 0);

  // Once the frame elapses, report 0% since there were no failures.
  task_environment_.FastForwardBy(base::Hours(1));
  histogram_tester_.ExpectUniqueSample(kBackupResultsFailuresHistogramName, 0,
                                       1);
}

TEST_F(BackupResultsMetricsTest, DoesNotReportWithoutAnyQueries) {
  task_environment_.FastForwardBy(base::Days(1));

  // No queries were made in the reporting frame, so nothing should report.
  histogram_tester_.ExpectTotalCount(kBackupResultsFailuresHistogramName, 0);
}

TEST_F(BackupResultsMetricsTest, ReportsBucketedFailurePercentage) {
  // 1 failure out of 4 queries -> 25% -> "1-25%" bucket (index 1).
  backup_results_metrics_->RecordQuery(true);
  backup_results_metrics_->RecordQuery(false);
  backup_results_metrics_->RecordQuery(false);
  backup_results_metrics_->RecordQuery(false);

  task_environment_.FastForwardBy(base::Days(1));

  histogram_tester_.ExpectUniqueSample(kBackupResultsFailuresHistogramName, 1,
                                       1);

  // 2 failures out of 4 queries -> 50% -> "26-50%" bucket (index 2).
  backup_results_metrics_->RecordQuery(true);
  backup_results_metrics_->RecordQuery(true);
  backup_results_metrics_->RecordQuery(false);
  backup_results_metrics_->RecordQuery(false);

  task_environment_.FastForwardBy(base::Days(1));

  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 2,
                                      1);

  // 3 failures out of 4 queries -> 75% -> "51-75%" bucket (index 3).
  backup_results_metrics_->RecordQuery(true);
  backup_results_metrics_->RecordQuery(true);
  backup_results_metrics_->RecordQuery(true);
  backup_results_metrics_->RecordQuery(false);

  task_environment_.FastForwardBy(base::Days(1));

  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 3,
                                      1);

  // All queries fail -> 100% -> "76-100%" bucket (index 4).
  backup_results_metrics_->RecordQuery(true);

  task_environment_.FastForwardBy(base::Days(1));

  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 4,
                                      1);

  histogram_tester_.ExpectTotalCount(kBackupResultsFailuresHistogramName, 4);
}

TEST_F(BackupResultsMetricsTest, SmallFailurePercentageIsNotRoundedToZero) {
  // 1 failure out of 1000 queries would round down to 0% with integer
  // division, but should still be reported as at least 1% (bucket index 1).
  backup_results_metrics_->RecordQuery(true);
  for (int i = 0; i < 999; i++) {
    backup_results_metrics_->RecordQuery(false);
  }

  task_environment_.FastForwardBy(base::Days(1));

  histogram_tester_.ExpectUniqueSample(kBackupResultsFailuresHistogramName, 1,
                                       1);
}

TEST_F(BackupResultsMetricsTest, ResetsCountsAfterReporting) {
  // First frame: 100% failures -> bucket index 4.
  backup_results_metrics_->RecordQuery(true);
  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 4,
                                      1);

  // Second frame: counts should have reset, so a single success reports 0%
  // (bucket index 0).
  backup_results_metrics_->RecordQuery(false);
  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 0,
                                      1);

  histogram_tester_.ExpectTotalCount(kBackupResultsFailuresHistogramName, 2);
}

TEST_F(BackupResultsMetricsTest, PersistsAcrossRestarts) {
  backup_results_metrics_->RecordQuery(true);
  histogram_tester_.ExpectTotalCount(kBackupResultsFailuresHistogramName, 0);

  // Simulate a browser restart; the accumulated counts and frame start time
  // should be preserved via prefs.
  CreateMetrics();

  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectUniqueSample(kBackupResultsFailuresHistogramName, 4,
                                       1);
}

}  // namespace brave_search
