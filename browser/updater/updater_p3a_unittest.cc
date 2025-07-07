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

  void SimulateLaunch(int day, std::string current_version);
  void ExpectReportedUpdate();
  void ExpectReportedNoUpdate();

  void ResetHistogramTester() {
    histogram_tester_ = std::make_unique<base::HistogramTester>();
  }

 private:
  void ExpectReportedStatus(UpdateStatus status);
};

TEST_P(UpdaterP3ATest, TestNoReportInInitialLaunch) {
  SimulateLaunch(0, "1.0.0.0");
  histogram_tester_->ExpectTotalCount(kUpdateStatusHistogramName, 0);
}

TEST_P(UpdaterP3ATest, TestReportsNoUpdate) {
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(1, "1.0.0.0");
  ExpectReportedNoUpdate();
}

TEST_P(UpdaterP3ATest, TestReportsUpdate) {
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(1, "2.0.0.0");
  ExpectReportedUpdate();
}

TEST_P(UpdaterP3ATest, TestReportsUpdateForOneWeek) {
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(1, "2.0.0.0");
  ResetHistogramTester();

  SimulateLaunch(7, "2.0.0.0");
  ExpectReportedUpdate();
}

TEST_P(UpdaterP3ATest, TestStopsReportingAfterOneWeek) {
  SimulateLaunch(0, "1.0.0.0");
  SimulateLaunch(1, "2.0.0.0");
  ResetHistogramTester();

  SimulateLaunch(8, "2.0.0.0");
  ExpectReportedNoUpdate();
}

INSTANTIATE_TEST_SUITE_P(
    // Empty to simplify gtest output
    ,
    UpdaterP3ATest,
    ::testing::Values(false, true),
    [](const ::testing::TestParamInfo<bool>& info) {
      return info.param ? "Omaha4" : "Legacy";
    });

void UpdaterP3ATest::SimulateLaunch(int day, std::string current_version) {
  base::Time now = base::Time::FromTimeT(1) + base::Days(day);
  ReportLaunch(now, current_version, IsUsingOmaha4(), &local_state_);
}

void UpdaterP3ATest::ExpectReportedUpdate() {
  using enum UpdateStatus;
  UpdateStatus status =
      IsUsingOmaha4() ? kUpdatedWithOmaha4 : kUpdatedWithLegacy;
  ExpectReportedStatus(status);
}

void UpdaterP3ATest::ExpectReportedNoUpdate() {
  using enum UpdateStatus;
  UpdateStatus status =
      IsUsingOmaha4() ? kNoUpdateWithOmaha4 : kNoUpdateWithLegacy;
  ExpectReportedStatus(status);
}

void UpdaterP3ATest::ExpectReportedStatus(UpdateStatus status) {
  histogram_tester_->ExpectUniqueSample(kUpdateStatusHistogramName, status, 1);
  ResetHistogramTester();
}

}  // namespace brave_updater
