/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/p3a_bandwidth_savings_permanent_state.h"

#include <memory>
#include <utility>

#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "brave/components/brave_perf_predictor/browser/p3a_bandwidth_savings_tracker.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_perf_predictor {

class P3ABandwidthSavingsPermanentStateTest : public ::testing::Test {
 public:
  P3ABandwidthSavingsPermanentStateTest() : clock_(new base::SimpleTestClock) {
    P3ABandwidthSavingsTracker::RegisterPrefs(pref_service_.registry());
    state_ = std::make_unique<P3ABandwidthSavingsPermanentState>(
        &pref_service_, std::unique_ptr<base::Clock>(clock_));
    clock_->SetNow(base::Time::Now());
  }

 protected:
  base::SimpleTestClock* clock_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<P3ABandwidthSavingsPermanentState> state_;
};

TEST_F(P3ABandwidthSavingsPermanentStateTest, StartsZero) {
  EXPECT_EQ(state_->GetFullPeriodSavingsBytes(), 0ULL);
}

TEST_F(P3ABandwidthSavingsPermanentStateTest, AddsSavings) {
  uint64_t saving = 10000;
  state_->AddSavings(saving);
  EXPECT_EQ(state_->GetFullPeriodSavingsBytes(), saving);

  // Accumulate
  state_->AddSavings(saving);
  state_->AddSavings(saving);
  EXPECT_EQ(state_->GetFullPeriodSavingsBytes(), saving * 3);
}

TEST_F(P3ABandwidthSavingsPermanentStateTest, ForgetsOldSavings) {
  uint64_t saving = 10000;
  state_->AddSavings(saving);
  EXPECT_EQ(state_->GetFullPeriodSavingsBytes(), saving);

  clock_->Advance(base::TimeDelta::FromDays(8));

  // More savings
  state_->AddSavings(saving);
  state_->AddSavings(saving);
  // Should have forgotten about older days
  EXPECT_EQ(state_->GetFullPeriodSavingsBytes(), saving * 2);
}

TEST_F(P3ABandwidthSavingsPermanentStateTest, RetrievesDailySavings) {
  uint64_t saving = 10000;
  for (int day = 0; day <= 7; day++) {
    clock_->Advance(base::TimeDelta::FromDays(1));
    state_->AddSavings(saving);
  }
  EXPECT_EQ(state_->GetFullPeriodSavingsBytes(), 7 * saving);
}

TEST_F(P3ABandwidthSavingsPermanentStateTest, HandlesSkippedDay) {
  uint64_t saving = 10000;
  for (int day = 0; day < 7; day++) {
    clock_->Advance(base::TimeDelta::FromDays(1));
    if (day == 3)
      continue;
    state_->AddSavings(saving);
  }
  EXPECT_EQ(state_->GetFullPeriodSavingsBytes(), 6 * saving);
}

TEST_F(P3ABandwidthSavingsPermanentStateTest, IntermittentUsage) {
  uint64_t saving = 10000;
  for (int day = 0; day < 10; day++) {
    clock_->Advance(base::TimeDelta::FromDays(2));
    state_->AddSavings(saving);
  }
  EXPECT_EQ(state_->GetFullPeriodSavingsBytes(), 4 * saving);
}

TEST_F(P3ABandwidthSavingsPermanentStateTest, InfrequentUsage) {
  uint64_t saving = 10000;
  state_->AddSavings(saving);
  clock_->Advance(base::TimeDelta::FromDays(6));
  state_->AddSavings(saving);
  EXPECT_EQ(state_->GetFullPeriodSavingsBytes(), 2 * saving);
}

}  // namespace brave_perf_predictor
