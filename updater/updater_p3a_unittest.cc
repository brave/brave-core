/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/updater/updater_p3a.h"

#include "base/test/metrics/histogram_tester.h"
#include "base/time/time.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_updater {

class BraveUpdaterP3ATest : public ::testing::TestWithParam<bool> {
 public:
  BraveUpdaterP3ATest() = default;
  ~BraveUpdaterP3ATest() override = default;

  void SetUp() override { RegisterLocalState(local_state_.registry()); }

 protected:
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;

  bool IsUsingOmaha4() const { return GetParam(); }

  void SimulateLaunch(int day, std::string current_version);
  void ExpectBucketCounts(int num_updates, int num_no_updates);
};

TEST_P(BraveUpdaterP3ATest, TestNoUpdate) {
  // No updates for 8 days:
  for (int day = 0; day <= 7; day++) {
    ExpectBucketCounts(0, 0);
    SimulateLaunch(day, "0.0.0.1");
  }
  // On the 8th day, the implementation should have reported that there was no
  // update in the previous week:
  ExpectBucketCounts(1, 0);
}

TEST_P(BraveUpdaterP3ATest, TestUpdated) {
  // Initial launch on the first day:
  SimulateLaunch(0, "0.0.0.1");
  // A new version on the second day, then no updates until the 14th day:
  for (int day = 1; day <= 13; day++) {
    SimulateLaunch(day, "0.0.0.2");
    // Should report the update immediately, but only once:
    ExpectBucketCounts(0, 1);
  }
  // On the 15th day, there was no update in the previous week. The
  // implementation should report this:
  SimulateLaunch(14, "0.0.0.2");
  ExpectBucketCounts(1, 1);
}

TEST_P(BraveUpdaterP3ATest, TestUpdatedComplex) {
  // Like TestUpdated, but with an update on the 15th day.

  // Initial launch:
  SimulateLaunch(0, "0.0.0.1");
  ExpectBucketCounts(0, 0);

  // An update in week 1:
  SimulateLaunch(1, "0.0.0.2");
  ExpectBucketCounts(0, 1);

  // No update at the beginning of week 2:
  SimulateLaunch(7, "0.0.0.2");
  ExpectBucketCounts(0, 1);

  // An update in week 3:
  SimulateLaunch(14, "0.0.0.3");
  ExpectBucketCounts(0, 2);
}

INSTANTIATE_TEST_SUITE_P(
    // Empty to simplify gtest output
    ,
    BraveUpdaterP3ATest,
    ::testing::Values(false, true),
    [](const ::testing::TestParamInfo<bool>& info) {
      return info.param ? "Omaha4" : "Legacy";
    });

void BraveUpdaterP3ATest::SimulateLaunch(int day, std::string current_version) {
  base::Time now = base::Time::FromTimeT(1) + base::Days(day);
  ReportLaunch(now, current_version, IsUsingOmaha4(), &local_state_);
}

void BraveUpdaterP3ATest::ExpectBucketCounts(int num_no_updates,
                                             int num_updates) {
  UpdateStatus no_update =
      IsUsingOmaha4() ? kNoUpdateWithOmaha4 : kNoUpdateWithLegacy;
  UpdateStatus updated =
      IsUsingOmaha4() ? kUpdatedWithOmaha4 : kUpdatedWithLegacy;
  histogram_tester_.ExpectBucketCount(kUpdateStatusHistogramName, no_update,
                                      num_no_updates);
  histogram_tester_.ExpectBucketCount(kUpdateStatusHistogramName, updated,
                                      num_updates);
  histogram_tester_.ExpectTotalCount(kUpdateStatusHistogramName,
                                     num_updates + num_no_updates);
}

}  // namespace brave_updater
