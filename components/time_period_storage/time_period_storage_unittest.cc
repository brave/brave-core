/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/time_period_storage/time_period_storage.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

constexpr char kPrefName[] = "brave.weekly_test";

class TimePeriodStorageTest : public ::testing::Test {
 public:
  TimePeriodStorageTest() {
    pref_service_.registry()->RegisterListPref(kPrefName);
  }

  void InitStorage(size_t days) {
    std::unique_ptr<base::SimpleTestClock> clock =
        std::make_unique<base::SimpleTestClock>();

    base::Time future_mock_time;
    // Set to fixed date to avoid DST related issues
    CHECK(base::Time::FromString("2050-01-04", &future_mock_time));
    clock->SetNow(future_mock_time.LocalMidnight() - base::Hours(4));
    clock_ = clock.get();

    state_ = std::make_unique<TimePeriodStorage>(&pref_service_, kPrefName,
                                                 days, std::move(clock));
  }

 protected:
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<TimePeriodStorage> state_;
  raw_ptr<base::SimpleTestClock> clock_ = nullptr;
};

TEST_F(TimePeriodStorageTest, StartsZero) {
  InitStorage(7);
  EXPECT_EQ(state_->GetPeriodSum(), 0ULL);
}

TEST_F(TimePeriodStorageTest, AddsSavings) {
  InitStorage(7);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), saving);

  // Accumulate
  state_->AddDelta(saving);
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), saving * 3);
}

TEST_F(TimePeriodStorageTest, SubDelta) {
  InitStorage(7);
  state_->AddDelta(5000);
  clock_->Advance(base::Days(1));
  state_->AddDelta(3000);
  clock_->Advance(base::Days(1));
  state_->AddDelta(1000);
  clock_->Advance(base::Days(1));

  state_->SubDelta(500);
  EXPECT_EQ(state_->GetPeriodSum(), 8500U);
  state_->SubDelta(4000);
  EXPECT_EQ(state_->GetPeriodSum(), 4500U);

  clock_->Advance(base::Days(4));
  // First day value should expire
  EXPECT_EQ(state_->GetPeriodSum(), 0U);

  // If subtracting by an amount greater than the current sum,
  // the sum should not become negative or underflow.
  state_->AddDelta(3000);
  state_->SubDelta(5000);
  EXPECT_EQ(state_->GetPeriodSum(), 0U);
  state_->SubDelta(100000);
  EXPECT_EQ(state_->GetPeriodSum(), 0U);
}

TEST_F(TimePeriodStorageTest, GetSumInCustomPeriod) {
  base::TimeDelta start_time_delta = base::Days(9) + base::Hours(1);
  base::TimeDelta end_time_delta = base::Days(4) - base::Hours(1);
  uint64_t saving = 10000;

  InitStorage(14);
  state_->AddDelta(saving);

  clock_->Advance(base::Days(1));
  state_->AddDelta(saving);
  state_->AddDelta(saving);

  clock_->Advance(base::Days(2));

  base::Time midnight = clock_->Now().LocalMidnight();
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight - start_time_delta,
                                            midnight - end_time_delta),
            0u);

  clock_->Advance(base::Days(1));
  midnight = clock_->Now().LocalMidnight();
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight - start_time_delta,
                                            midnight - end_time_delta),
            saving);

  clock_->Advance(base::Days(1));
  midnight = clock_->Now().LocalMidnight();
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight - start_time_delta,
                                            midnight - end_time_delta),
            saving * 3);

  clock_->Advance(base::Days(5));
  midnight = clock_->Now().LocalMidnight();
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight - start_time_delta,
                                            midnight - end_time_delta),
            saving * 2);

  clock_->Advance(base::Days(1));
  midnight = clock_->Now().LocalMidnight();
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight - start_time_delta,
                                            midnight - end_time_delta),
            0u);
}

TEST_F(TimePeriodStorageTest, ForgetsOldSavingsWeekly) {
  InitStorage(7);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), saving);

  clock_->Advance(base::Days(8));

  // More savings
  state_->AddDelta(saving);
  state_->AddDelta(saving);
  // Should have forgotten about older days
  EXPECT_EQ(state_->GetPeriodSum(), saving * 2);
}

TEST_F(TimePeriodStorageTest, ForgetsOldSavingsMonthly) {
  InitStorage(30);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), saving);

  clock_->Advance(base::Days(31));

  // More savings
  state_->AddDelta(saving);
  state_->AddDelta(saving);
  // Should have forgotten about older days
  EXPECT_EQ(state_->GetPeriodSum(), saving * 2);
}

TEST_F(TimePeriodStorageTest, RetrievesDailySavings) {
  InitStorage(7);
  uint64_t saving = 10000;
  for (int day = 0; day <= 7; day++) {
    clock_->Advance(base::Days(1));
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetPeriodSum(), 7 * saving);
}

TEST_F(TimePeriodStorageTest, HandlesSkippedDay) {
  InitStorage(7);
  uint64_t saving = 10000;
  for (int day = 0; day < 7; day++) {
    clock_->Advance(base::Days(1));
    if (day == 3)
      continue;
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetPeriodSum(), 6 * saving);
}

TEST_F(TimePeriodStorageTest, IntermittentUsageWeekly) {
  InitStorage(7);
  uint64_t saving = 10000;
  for (int day = 0; day < 10; day++) {
    clock_->Advance(base::Days(2));
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetPeriodSum(), 4 * saving);
}

TEST_F(TimePeriodStorageTest, IntermittentUsageMonthly) {
  InitStorage(30);
  uint64_t saving = 10000;
  for (int day = 0; day < 40; day++) {
    clock_->Advance(base::Days(10));
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetPeriodSum(), 3 * saving);
}

TEST_F(TimePeriodStorageTest, InfrequentUsageWeekly) {
  InitStorage(7);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  clock_->Advance(base::Days(6));
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), 2 * saving);
}

TEST_F(TimePeriodStorageTest, InfrequentUsageMonthly) {
  InitStorage(30);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  clock_->Advance(base::Days(29));
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), 2 * saving);
}

TEST_F(TimePeriodStorageTest, GetHighestValueInPeriod) {
  InitStorage(7);
  uint64_t lowest_value = 20;
  uint64_t low_value = 50;
  uint64_t high_value = 75;
  state_->AddDelta(low_value);
  clock_->Advance(base::Days(1));
  state_->AddDelta(high_value);
  clock_->Advance(base::Days(1));
  state_->AddDelta(lowest_value);
  EXPECT_EQ(state_->GetHighestValueInPeriod(), high_value);
  clock_->Advance(base::Days(1));
  EXPECT_EQ(state_->GetHighestValueInPeriod(), high_value);
}

TEST_F(TimePeriodStorageTest, RecordsHigherValueForToday) {
  InitStorage(30);
  uint64_t low_value = 50;
  uint64_t high_value = 75;
  state_->ReplaceTodaysValueIfGreater(low_value);
  EXPECT_EQ(state_->GetHighestValueInPeriod(), low_value);
  // Replace with higher value
  state_->ReplaceTodaysValueIfGreater(high_value);
  EXPECT_EQ(state_->GetHighestValueInPeriod(), high_value);
  // Sanity check value was replaced and not added.
  EXPECT_EQ(state_->GetPeriodSum(), high_value);
  // Should not replace with lower value
  state_->ReplaceTodaysValueIfGreater(low_value);
  EXPECT_EQ(state_->GetHighestValueInPeriod(), high_value);
}

TEST_F(TimePeriodStorageTest, GetsHighestValueInWeekFromReplacement) {
  InitStorage(30);
  // Add a low value a couple days after a high value,
  // should return highest day value.
  uint64_t low_value = 50;
  uint64_t high_value = 75;
  state_->ReplaceTodaysValueIfGreater(high_value);
  clock_->Advance(base::Days(2));
  state_->ReplaceTodaysValueIfGreater(low_value);
  EXPECT_EQ(state_->GetHighestValueInPeriod(), high_value);
  // Sanity check disparate days were not replaced
  EXPECT_EQ(state_->GetPeriodSum(), high_value + low_value);
}

TEST_F(TimePeriodStorageTest, ReplaceIfGreaterForDate) {
  InitStorage(30);

  state_->AddDelta(4);
  clock_->Advance(base::Days(1));
  state_->AddDelta(2);
  clock_->Advance(base::Days(1));
  state_->AddDelta(1);
  clock_->Advance(base::Days(1));

  // should replace
  state_->ReplaceIfGreaterForDate(clock_->Now() - base::Days(2), 3);
  // should not replace
  state_->ReplaceIfGreaterForDate(clock_->Now() - base::Days(3), 3);

  EXPECT_EQ(state_->GetPeriodSum(), 8U);

  // should insert new daily value
  state_->ReplaceIfGreaterForDate(clock_->Now() - base::Days(4), 3);
  EXPECT_EQ(state_->GetPeriodSum(), 11U);

  // should store, but should not be in sum because it's too old
  state_->ReplaceIfGreaterForDate(clock_->Now() - base::Days(31), 10);
  EXPECT_EQ(state_->GetPeriodSum(), 11U);
}
