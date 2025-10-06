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
  }

  void CreateMetrics() {
    backup_results_metrics_ = std::make_unique<BackupResultsMetrics>(&prefs_);
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<BackupResultsMetrics> backup_results_metrics_;
  base::HistogramTester histogram_tester_;
};

TEST_F(BackupResultsMetricsTest, RecordsSuccessfulQuery) {
  CreateMetrics();

  histogram_tester_.ExpectTotalCount(kBackupResultsFailuresHistogramName, 0);

  // Record a successful query
  backup_results_metrics_->RecordQuery(false);

  // Should report 0 failures
  histogram_tester_.ExpectUniqueSample(kBackupResultsFailuresHistogramName, 0,
                                       1);
}

TEST_F(BackupResultsMetricsTest, RecordsFailedQuery) {
  CreateMetrics();

  // Record a failed query
  backup_results_metrics_->RecordQuery(true);

  // Should report 1 failure
  histogram_tester_.ExpectUniqueSample(kBackupResultsFailuresHistogramName, 1,
                                       1);

  backup_results_metrics_->RecordQuery(true);
  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 2,
                                      1);

  // Record multiple failed queries
  for (size_t i = 0; i < 3; i++) {
    backup_results_metrics_->RecordQuery(true);
  }

  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 3,
                                      3);
  histogram_tester_.ExpectTotalCount(kBackupResultsFailuresHistogramName, 5);
}

TEST_F(BackupResultsMetricsTest, RecordsMixedQueries) {
  CreateMetrics();

  // Record mix of successful and failed queries
  backup_results_metrics_->RecordQuery(false);  // success - reports 0 failures
  backup_results_metrics_->RecordQuery(true);   // failure - reports 1 failure
  backup_results_metrics_->RecordQuery(
      false);  // success - reports 1 failure (unchanged)
  backup_results_metrics_->RecordQuery(true);  // failure - reports 2 failures

  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 0,
                                      1);
  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 1,
                                      2);  // 1 intermediate report
  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 2,
                                      1);  // final report
  histogram_tester_.ExpectTotalCount(kBackupResultsFailuresHistogramName, 4);
}

TEST_F(BackupResultsMetricsTest, CanResetMetricsDuringTest) {
  CreateMetrics();

  // Record a query
  backup_results_metrics_->RecordQuery(true);
  histogram_tester_.ExpectUniqueSample(kBackupResultsFailuresHistogramName, 1,
                                       1);

  CreateMetrics();

  histogram_tester_.ExpectUniqueSample(kBackupResultsFailuresHistogramName, 1,
                                       2);

  backup_results_metrics_->RecordQuery(false);

  // Should report 0 failures for the new instance
  histogram_tester_.ExpectUniqueSample(kBackupResultsFailuresHistogramName, 1,
                                       3);
}

TEST_F(BackupResultsMetricsTest, HistogramUpdatedAfterTimerFires) {
  CreateMetrics();

  // Record a successful query first to establish last query time
  backup_results_metrics_->RecordQuery(false);
  histogram_tester_.ExpectUniqueSample(kBackupResultsFailuresHistogramName, 0,
                                       1);

  task_environment_.FastForwardBy(base::Hours(1));

  histogram_tester_.ExpectUniqueSample(kBackupResultsFailuresHistogramName, 0,
                                       2);

  // Record a failure
  backup_results_metrics_->RecordQuery(true);
  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 1,
                                      1);

  // Fast forward by 1 hour (within 24 hours)
  task_environment_.FastForwardBy(base::Hours(1));

  // Timer should have fired and reported the metric again since within 24h
  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 1,
                                      2);

  task_environment_.FastForwardBy(base::Days(2));

  // Histogram should NOT update anymore since last query time exceeds 24 hours
  histogram_tester_.ExpectBucketCount(kBackupResultsFailuresHistogramName, 1,
                                      24);
  histogram_tester_.ExpectTotalCount(kBackupResultsFailuresHistogramName,
                                     26);  // No change

  CreateMetrics();
  histogram_tester_.ExpectTotalCount(kBackupResultsFailuresHistogramName,
                                     26);  // No change
}

}  // namespace brave_search
