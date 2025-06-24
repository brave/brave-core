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

class BraveUpdaterP3ATest : public ::testing::Test {
 public:
  BraveUpdaterP3ATest() = default;
  ~BraveUpdaterP3ATest() override = default;

  void SetUp() override {
    RegisterLocalState(local_state_.registry());
  }

 protected:
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;

  void TestNoUpdate(bool is_using_omaha4);
  void TestUpdated(bool is_using_omaha4);

  UpdateStatus GetExpectedStatus(bool is_using_omaha4, bool updated);

  void SimulateLaunch(int day, std::string current_version,
                      bool is_using_omaha4);
  void ExpectBucketCount(int sample, int count);
  void ExpectTotalCount(int count);
};

TEST_F(BraveUpdaterP3ATest, TestNoUpdateWithLegacy) {
  TestNoUpdate(false);
}

TEST_F(BraveUpdaterP3ATest, TestNoUpdateWithOmaha4) {
  TestNoUpdate(true);
}

TEST_F(BraveUpdaterP3ATest, TestUpdatedWithLegacy) {
  TestUpdated(false);
}

TEST_F(BraveUpdaterP3ATest, TestUpdatedWithOmaha4) {
  TestUpdated(true);
}

void BraveUpdaterP3ATest::TestNoUpdate(bool is_using_omaha4) {
  // No updates for 8 days:
  for (int day = 0; day <= 7; day++) {
    ExpectTotalCount(0);
    SimulateLaunch(day, "0.0.0.1", is_using_omaha4);
  }
  // On the 8th day, the implementation should have reported that there was no
  // update in the previous week:
  ExpectTotalCount(1);
  UpdateStatus no_update = GetExpectedStatus(is_using_omaha4, false);
  ExpectBucketCount(no_update, 1);
}

void BraveUpdaterP3ATest::TestUpdated(bool is_using_omaha4) {
  // Initial launch on the first day:
  SimulateLaunch(0, "0.0.0.1", is_using_omaha4);
  UpdateStatus no_update = GetExpectedStatus(is_using_omaha4, false);
  UpdateStatus updated = GetExpectedStatus(is_using_omaha4, true);
  // A new version on the second day, then no updates until the 14th day:
  for (int day = 1; day <= 13; day++) {
    SimulateLaunch(day, "0.0.0.2", is_using_omaha4);
    // Should report the update immediately, but only once:
    ExpectTotalCount(1);
    ExpectBucketCount(updated, 1);
    ExpectBucketCount(no_update, 0);
  }
  // On the 15th day, there was no update in the previous week. The
  // implementation should report this:
  SimulateLaunch(14, "0.0.0.2", is_using_omaha4);
  ExpectTotalCount(2);
  ExpectBucketCount(updated, 1);
  ExpectBucketCount(no_update, 1);
}

UpdateStatus BraveUpdaterP3ATest::GetExpectedStatus(bool is_using_omaha4,
                                                    bool updated) {
  return updated ? (is_using_omaha4 ? kUpdatedWithOmaha4 : kUpdatedWithLegacy)
                 : (is_using_omaha4 ? kNoUpdateWithOmaha4 : kNoUpdateWithLegacy);
}

void BraveUpdaterP3ATest::SimulateLaunch(int day, std::string current_version,
                                         bool is_using_omaha4) {
  base::Time now = base::Time::FromTimeT(1) + base::Days(day);
  ReportLaunch(now, current_version, is_using_omaha4, &local_state_);
}

void BraveUpdaterP3ATest::ExpectTotalCount(int count) {
  histogram_tester_.ExpectTotalCount(kUpdateStatusHistogramName, count);
}

void BraveUpdaterP3ATest::ExpectBucketCount(int sample, int count) {
  histogram_tester_.ExpectBucketCount(kUpdateStatusHistogramName, sample,
                                       count);
}

}  // namespace brave_updater
