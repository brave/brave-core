/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/p3a_bandwidth_savings_tracker.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_perf_predictor {

constexpr char kSavingsDailyUMAHistogramName[] =
    "Brave.Savings.BandwidthSavingsMB";

class P3ABandwidthSavingsTrackerTest : public ::testing::Test {
 public:
  P3ABandwidthSavingsTrackerTest() : clock_(new base::SimpleTestClock) {
    P3ABandwidthSavingsTracker::RegisterProfilePrefs(pref_service_.registry());
    tracker_ = std::make_unique<P3ABandwidthSavingsTracker>(
        &pref_service_, std::unique_ptr<base::Clock>(clock_));
    clock_->SetNow(base::Time::Now());
  }

 protected:
  raw_ptr<base::SimpleTestClock> clock_ = nullptr;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<P3ABandwidthSavingsTracker> tracker_;
};

TEST_F(P3ABandwidthSavingsTrackerTest, RecordSavingsHistogram) {
  base::HistogramTester tester;
  tracker_->RecordSavings(10 << 20);
  tester.ExpectBucketCount(kSavingsDailyUMAHistogramName, 1, 1);
}

TEST_F(P3ABandwidthSavingsTrackerTest, RecordSavingsHistogramIncrements) {
  base::HistogramTester tester;
  tracker_->RecordSavings(10 << 20);
  tracker_->RecordSavings(20 << 20);
  tracker_->RecordSavings(5 << 20);
  tracker_->RecordSavings(20 << 20);
  tester.ExpectBucketCount(kSavingsDailyUMAHistogramName, 2, 1);
}

TEST_F(P3ABandwidthSavingsTrackerTest, RecordSavingsHistogramLargeIncrements) {
  base::HistogramTester tester;
  tracker_->RecordSavings(10 << 20);
  tracker_->RecordSavings(700 << 20);
  tester.ExpectBucketCount(kSavingsDailyUMAHistogramName, 6, 1);
}

}  // namespace brave_perf_predictor
