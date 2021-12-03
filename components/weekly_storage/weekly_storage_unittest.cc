/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/weekly_storage/weekly_storage.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class WeeklyStorageTest : public ::testing::Test {
 public:
  WeeklyStorageTest() : clock_(new base::SimpleTestClock) {
    constexpr char kPrefName[] = "brave.weekly_test";
    pref_service_.registry()->RegisterListPref(kPrefName);

    state_ = std::make_unique<WeeklyStorage>(
        &pref_service_, kPrefName, std::unique_ptr<base::Clock>(clock_));
    clock_->SetNow(base::Time::Now());
  }

 protected:
  raw_ptr<base::SimpleTestClock> clock_ = nullptr;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<WeeklyStorage> state_;
};

TEST_F(WeeklyStorageTest, StartsZero) {
  EXPECT_EQ(state_->GetWeeklySum(), 0ULL);
}

TEST_F(WeeklyStorageTest, AddsSavings) {
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetWeeklySum(), saving);

  // Accumulate
  state_->AddDelta(saving);
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetWeeklySum(), saving * 3);
}

TEST_F(WeeklyStorageTest, ForgetsOldSavings) {
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetWeeklySum(), saving);

  clock_->Advance(base::Days(8));

  // More savings
  state_->AddDelta(saving);
  state_->AddDelta(saving);
  // Should have forgotten about older days
  EXPECT_EQ(state_->GetWeeklySum(), saving * 2);
}

TEST_F(WeeklyStorageTest, RetrievesDailySavings) {
  uint64_t saving = 10000;
  for (int day = 0; day <= 7; day++) {
    clock_->Advance(base::Days(1));
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetWeeklySum(), 7 * saving);
}

TEST_F(WeeklyStorageTest, HandlesSkippedDay) {
  uint64_t saving = 10000;
  for (int day = 0; day < 7; day++) {
    clock_->Advance(base::Days(1));
    if (day == 3)
      continue;
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetWeeklySum(), 6 * saving);
}

TEST_F(WeeklyStorageTest, IntermittentUsage) {
  uint64_t saving = 10000;
  for (int day = 0; day < 10; day++) {
    clock_->Advance(base::Days(2));
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetWeeklySum(), 4 * saving);
}

TEST_F(WeeklyStorageTest, InfrequentUsage) {
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  clock_->Advance(base::Days(6));
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetWeeklySum(), 2 * saving);
}

TEST_F(WeeklyStorageTest, GetHighestValueInWeek) {
  uint64_t lowest_value = 20;
  uint64_t low_value = 50;
  uint64_t high_value = 75;
  state_->AddDelta(low_value);
  clock_->Advance(base::Days(1));
  state_->AddDelta(high_value);
  clock_->Advance(base::Days(1));
  state_->AddDelta(lowest_value);
  EXPECT_EQ(state_->GetHighestValueInWeek(), high_value);
  clock_->Advance(base::Days(1));
  EXPECT_EQ(state_->GetHighestValueInWeek(), high_value);
}

TEST_F(WeeklyStorageTest, RecordsHigherValueForToday) {
  uint64_t low_value = 50;
  uint64_t high_value = 75;
  state_->ReplaceTodaysValueIfGreater(low_value);
  EXPECT_EQ(state_->GetHighestValueInWeek(), low_value);
  // Replace with higher value
  state_->ReplaceTodaysValueIfGreater(high_value);
  EXPECT_EQ(state_->GetHighestValueInWeek(), high_value);
  // Sanity check value was replaced and not added.
  EXPECT_EQ(state_->GetWeeklySum(), high_value);
  // Should not replace with lower value
  state_->ReplaceTodaysValueIfGreater(low_value);
  EXPECT_EQ(state_->GetHighestValueInWeek(), high_value);
}

TEST_F(WeeklyStorageTest, GetsHighestValueInWeekFromReplacement) {
  // Add a low value a couple days after a high value,
  // should return highest day value.
  uint64_t low_value = 50;
  uint64_t high_value = 75;
  state_->ReplaceTodaysValueIfGreater(high_value);
  clock_->Advance(base::Days(2));
  state_->ReplaceTodaysValueIfGreater(low_value);
  EXPECT_EQ(state_->GetHighestValueInWeek(), high_value);
  // Sanity check disparate days were not replaced
  EXPECT_EQ(state_->GetWeeklySum(), high_value + low_value);
}
