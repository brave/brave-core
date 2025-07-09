/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/updater/updater_p3a.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/time/time.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_updater {

class UpdaterP3ATest : public ::testing::TestWithParam<bool> {
 public:
  UpdaterP3ATest() = default;
  ~UpdaterP3ATest() override = default;

  void SetUp() override {
    RegisterLocalStatePrefs(local_state_.registry());
    histogram_tester_ = std::make_unique<base::HistogramTester>();
  }

 protected:
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;

  bool IsUsingOmaha4() const { return GetParam(); }

  void SimulateLaunch(int day, std::string current_version) {
    base::Time now = base::Time::FromTimeT(1) + base::Days(day);
    ReportLaunch(now, current_version, IsUsingOmaha4(), &local_state_);
  }

  UpdateStatus StatusUpdate() {
    return IsUsingOmaha4() ? UpdateStatus::kUpdatedWithOmaha4
                           : UpdateStatus::kUpdatedWithLegacy;
  }

  UpdateStatus StatusNoUpdate() {
    return IsUsingOmaha4() ? UpdateStatus::kNoUpdateWithOmaha4
                           : UpdateStatus::kNoUpdateWithLegacy;
  }

  void ResetHistogramTester() {
    histogram_tester_ = std::make_unique<base::HistogramTester>();
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

TEST_P(UpdaterP3ATest, TestReportsUpdateIfVersionChanges) {
  // patch version change
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(1, "1.0.0.1");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);
  ResetHistogramTester();

  // build version change
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(1, "1.0.1.0");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);
  ResetHistogramTester();

  // minor version change
  SimulateLaunch(0, "2.0.0.0");
  SimulateLaunch(1, "2.1.0.0");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);
  ResetHistogramTester();

  // major version change
  SimulateLaunch(0, "2.0.0.0");
  SimulateLaunch(1, "3.0.0.0");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);
  ResetHistogramTester();

  // Install older version
  SimulateLaunch(0, "2.0.0.0");
  SimulateLaunch(1, "1.0.0.0");
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                        StatusUpdate(), 1);
  ResetHistogramTester();
}

TEST_P(UpdaterP3ATest, TestReportsUpdateForOneWeekIfVersionChanges) {
  for (int i = 1; i <= 7; i++) {
    SimulateLaunch(0, "1.0.0.0");
    SimulateLaunch(i, "2.0.0.0");
    histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName,
                                          StatusUpdate(), 1);
    ResetHistogramTester();
  }
}

TEST_P(UpdaterP3ATest, TestStopReportingAfterOneWeek) {
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(1, "2.0.0.0");
  ResetHistogramTester();

  SimulateLaunch(8, "2.0.0.0");
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
