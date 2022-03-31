// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/weekly_storage/daily_storage.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class DailyStorageTest : public ::testing::Test {
 public:
  DailyStorageTest() : clock_(new base::SimpleTestClock) {
    constexpr char kPrefName[] = "brave.daily_test";
    pref_service_.registry()->RegisterListPref(kPrefName);

    state_ = std::make_unique<DailyStorage>(
        &pref_service_, kPrefName, std::unique_ptr<base::Clock>(clock_));
    clock_->SetNow(base::Time::Now());
  }

 protected:
  raw_ptr<base::SimpleTestClock> clock_ = nullptr;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<DailyStorage> state_;
};

TEST_F(DailyStorageTest, StartsZero) {
  EXPECT_EQ(state_->GetLast24HourSum(), 0ULL);
}

TEST_F(DailyStorageTest, RecordsValue) {
  uint64_t value = 10000;
  state_->RecordValueNow(value);
  EXPECT_EQ(state_->GetLast24HourSum(), value);

  // Accumulate
  state_->RecordValueNow(value);
  state_->RecordValueNow(value);
  EXPECT_EQ(state_->GetLast24HourSum(), value * 3);
}

TEST_F(DailyStorageTest, ForgetsOldValues) {
  uint64_t value = 10000;
  state_->RecordValueNow(value);
  EXPECT_EQ(state_->GetLast24HourSum(), value);

  clock_->Advance(base::Hours(24));

  // More values
  state_->RecordValueNow(value);
  state_->RecordValueNow(value);
  // Should have forgotten about older days
  EXPECT_EQ(state_->GetLast24HourSum(), value * 2);
}

TEST_F(DailyStorageTest, RetrievesAllValues) {
  uint64_t value = 10000;
  for (int hour = 0; hour <= 24; hour++) {
    clock_->Advance(base::Hours(1));
    state_->RecordValueNow(value);
  }
  EXPECT_EQ(state_->GetLast24HourSum(), 24 * value);
}

TEST_F(DailyStorageTest, HandlesSkippedHour) {
  uint64_t value = 10000;
  for (int hour = 0; hour < 24; hour++) {
    clock_->Advance(base::Hours(1));
    if (hour == 3)
      continue;
    state_->RecordValueNow(value);
  }
  EXPECT_EQ(state_->GetLast24HourSum(), 23 * value);
}

TEST_F(DailyStorageTest, IntermittentUsage) {
  uint64_t value = 10000;
  for (int hour = 0; hour < 36; hour++) {
    clock_->Advance(base::Hours(2));
    state_->RecordValueNow(value);
  }
  EXPECT_EQ(state_->GetLast24HourSum(), 12 * value);
}

TEST_F(DailyStorageTest, InfrequentUsage) {
  uint64_t value = 10000;
  state_->RecordValueNow(value);
  clock_->Advance(base::Hours(23));
  state_->RecordValueNow(value);
  EXPECT_EQ(state_->GetLast24HourSum(), 2 * value);
}
