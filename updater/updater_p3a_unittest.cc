/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/updater/updater_p3a.h"

#include "base/time/time.h"
#include "base/test/metrics/histogram_tester.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_updater {

class BraveUpdaterP3ATest : public ::testing::TestWithParam<bool> {
 public:
  BraveUpdaterP3ATest() = default;
  ~BraveUpdaterP3ATest() override = default;

  void SetUp() override {
    RegisterLocalState(local_state_.registry());
  }

  bool IsUsingOmaha4() const { return GetParam(); }

 protected:
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;

  void TestNoUpdate();
  void TestUpdated();
  void TestUpdatedComplex();

  UpdateStatus GetExpectedStatus(bool updated);

  void SimulateLaunch(int day, std::string current_version);
  void ExpectBucketCount(int sample, int count);
  void ExpectTotalCount(int count);
};

TEST_P(BraveUpdaterP3ATest, TestNoUpdate) {
  TestNoUpdate();
}

TEST_P(BraveUpdaterP3ATest, TestUpdated) {
  TestUpdated();
}

TEST_P(BraveUpdaterP3ATest, TestUpdatedComplex) {
  TestUpdatedComplex();
}

INSTANTIATE_TEST_SUITE_P(
    // Empty to simplify gtest output
    ,
    BraveUpdaterP3ATest,
    ::testing::Values(false, true),
    [](const ::testing::TestParamInfo<bool>& info) {
      return info.param ? "Omaha4" : "Legacy";
    });

void BraveUpdaterP3ATest::TestNoUpdate() {
  // No updates for 8 days:
  for (int day = 0; day <= 7; day++) {
    ExpectTotalCount(0);
    SimulateLaunch(day, "0.0.0.1");
  }
  // On the 8th day, the implementation should have reported that there was no
  // update in the previous week:
  ExpectTotalCount(1);
  UpdateStatus no_update = GetExpectedStatus(false);
  ExpectBucketCount(no_update, 1);
}

void BraveUpdaterP3ATest::TestUpdated() {
  // Initial launch on the first day:
  SimulateLaunch(0, "0.0.0.1");
  UpdateStatus updated = GetExpectedStatus(true);
  UpdateStatus no_update = GetExpectedStatus(false);
  // A new version on the second day, then no updates until the 14th day:
  for (int day = 1; day <= 13; day++) {
    SimulateLaunch(day, "0.0.0.2");
    // Should report the update immediately, but only once:
    ExpectTotalCount(1);
    ExpectBucketCount(updated, 1);
    ExpectBucketCount(no_update, 0);
  }
  // On the 15th day, there was no update in the previous week. The
  // implementation should report this:
  SimulateLaunch(14, "0.0.0.2");
  ExpectTotalCount(2);
  ExpectBucketCount(updated, 1);
  ExpectBucketCount(no_update, 1);
}

void BraveUpdaterP3ATest::TestUpdatedComplex() {
  // Like TestUpdated, but with an update on the 15th day.
  UpdateStatus updated = GetExpectedStatus(true);

  // Initial launch:
  SimulateLaunch(0, "0.0.0.1");
  ExpectBucketCount(updated, 0);

  // An update in week 1:
  SimulateLaunch(1, "0.0.0.2");
  ExpectBucketCount(updated, 1);

  // No update at the beginning of week 2:
  SimulateLaunch(7, "0.0.0.2");
  ExpectBucketCount(updated, 1);

  // An update in week 3:
  SimulateLaunch(14, "0.0.0.3");
  ExpectBucketCount(updated, 2);
 
  ExpectTotalCount(2);
}

UpdateStatus BraveUpdaterP3ATest::GetExpectedStatus(bool updated) {
  return updated ? (IsUsingOmaha4() ? kUpdatedWithOmaha4 : kUpdatedWithLegacy)
                 : (IsUsingOmaha4() ? kNoUpdateWithOmaha4 : kNoUpdateWithLegacy);
}

void BraveUpdaterP3ATest::SimulateLaunch(int day, std::string current_version) {
  base::Time now = base::Time::FromTimeT(1) + base::Days(day);
  ReportLaunch(now, current_version, IsUsingOmaha4(), &local_state_);
}

void BraveUpdaterP3ATest::ExpectTotalCount(int count) {
  histogram_tester_.ExpectTotalCount(kUpdateStatusHistogramName, count);
}

void BraveUpdaterP3ATest::ExpectBucketCount(int sample, int count) {
  histogram_tester_.ExpectBucketCount(kUpdateStatusHistogramName, sample,
                                       count);
}

}  // namespace brave_updater
