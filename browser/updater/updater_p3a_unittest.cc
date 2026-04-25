/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/updater/updater_p3a.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_updater {

class UpdaterP3ATest : public ::testing::TestWithParam<bool> {
 public:
  UpdaterP3ATest() = default;
  ~UpdaterP3ATest() override = default;

  void SetUp() override {
    ResetHistogramTester();
    RegisterLocalStatePrefs(local_state_.registry());
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;

  bool IsUsingOmaha4() const { return GetParam(); }

  void SimulateLaunch(int advance_by_days, std::string current_version) {
    task_environment_.AdvanceClock(base::Days(advance_by_days));
    ReportLaunch(current_version, IsUsingOmaha4(), &local_state_);
  }

  void ResetHistogramTester() {
    histogram_tester_ = std::make_unique<base::HistogramTester>();
  }

  UpdateStatus StatusUpdate() {
    return IsUsingOmaha4() ? UpdateStatus::kUpdatedWithOmaha4
                           : UpdateStatus::kUpdatedWithLegacy;
  }

  UpdateStatus StatusNoUpdate() {
    return IsUsingOmaha4() ? UpdateStatus::kNoUpdateWithOmaha4
                           : UpdateStatus::kNoUpdateWithLegacy;
  }
};

TEST_P(UpdaterP3ATest, TestNoReportInInitialLaunch) {
  SimulateLaunch(0, "1.0.0.0");
  histogram_tester_->ExpectTotalCount(kUpdateStatusHistogramName, 0);
}

TEST_P(UpdaterP3ATest, TestReportNoUpdateIfNoVersionChange) {
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(1, "1.0.0.0");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusNoUpdate(), 1);
}

TEST_P(UpdaterP3ATest, TestReportsUpdateIfPatchVersionChanges) {
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(1, "1.0.0.1");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);
}

TEST_P(UpdaterP3ATest, TestReportsUpdateIfBuildVersionChanges) {
  SimulateLaunch(0, "1.0.0.100");
  SimulateLaunch(1, "1.0.1.0");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);
}

TEST_P(UpdaterP3ATest, TestReportsUpdateIfMinorVersionChanges) {
  SimulateLaunch(0, "2.0.100.0");
  SimulateLaunch(1, "2.1.0.0");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);
}

TEST_P(UpdaterP3ATest, TestReportsUpdateIfMajorVersionChanges) {
  SimulateLaunch(0, "2.100.0.0");
  SimulateLaunch(1, "3.0.0.0");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);
}

TEST_P(UpdaterP3ATest, TestReportsUpdateIfVersionDowngrade) {
  SimulateLaunch(0, "2.0.0.0");
  SimulateLaunch(1, "1.0.0.0");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);
}

TEST_P(UpdaterP3ATest, TestReportsUpdateIfVersionChangeSameDay) {
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(0, "1.0.0.1");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);
}

TEST_P(UpdaterP3ATest, TestReportsUpdateIfMulitpleVersionChanges) {
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(1, "1.0.0.1");
  SimulateLaunch(2, "1.0.0.2");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 2);
}

TEST_P(UpdaterP3ATest, TestReportsUpdateIfMulitpleVersionChangeSameDay) {
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(0, "1.0.0.1");
  SimulateLaunch(0, "1.0.0.2");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 2);
}

TEST_P(UpdaterP3ATest, TestReportsUpdateForSevenDaysIfVersionChanges) {
  SimulateLaunch(0, "1.0.0.0");
  for (int i = 1; i <= 7; i++) {
    SimulateLaunch(1, "2.0.0.0");
    histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                          StatusUpdate(), i);
  }
}

TEST_P(UpdaterP3ATest, TestStopReportingAfterSevenDays) {
  SimulateLaunch(0, "1.0.0.0");
  // Report 1 day after first launch
  SimulateLaunch(1, "2.0.0.0");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);

  ResetHistogramTester();
  // 1 + 7 days after first launch
  SimulateLaunch(7, "2.0.0.0");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusNoUpdate(), 1);
}

INSTANTIATE_TEST_SUITE_P(
    // Empty to simplify gtest output
    ,
    UpdaterP3ATest,
    ::testing::Values(false, true),
    [](const ::testing::TestParamInfo<bool>& info) {
      return info.param ? "Omaha4" : "Legacy";
    });

}  // namespace brave_updater
