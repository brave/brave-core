/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_storage.h"

#include <memory>

#include "base/check.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_pref_time_period_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

constexpr char kListPrefName[] = "brave.weekly_test";
constexpr char kDictPrefName[] = "brave.weekly_dict_test";
constexpr char kDictKey1[] = "key1";
constexpr char kDictKey2[] = "key2";

class SerpMetricsTimePeriodStorageTest : public ::testing::Test {
 public:
  SerpMetricsTimePeriodStorageTest() {
    pref_service_.registry()->RegisterListPref(kListPrefName);
    pref_service_.registry()->RegisterDictionaryPref(kDictPrefName);

    // Advance to a fixed date to avoid DST-related issues. 4 hours before
    // midnight gives tests room to advance forward within the same day.
    base::Time future_mock_time;
    CHECK(base::Time::FromString("2050-01-04", &future_mock_time));
    task_environment_.AdvanceClock(Midnight(future_mock_time) - base::Hours(4) -
                                   base::Time::Now());
  }

  void InitStorage(size_t days, const char* dict_key = nullptr) {
    const char* pref_name = dict_key ? kDictPrefName : kListPrefName;
    state_ = std::make_unique<SerpMetricsTimePeriodStorage>(
        std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                         pref_name, dict_key),
        days);
  }

  static base::Time Midnight(base::Time time) { return time.LocalMidnight(); }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<SerpMetricsTimePeriodStorage> state_;
};

TEST_F(SerpMetricsTimePeriodStorageTest, StartsZero) {
  InitStorage(7);
  EXPECT_EQ(state_->GetPeriodSum(), 0ULL);
}

TEST_F(SerpMetricsTimePeriodStorageTest, AddsSavings) {
  InitStorage(7);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), saving);

  // Accumulate
  state_->AddDelta(saving);
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), saving * 3);
}

TEST_F(SerpMetricsTimePeriodStorageTest, GetSumInCustomPeriod) {
  base::TimeDelta start_time_delta = base::Days(9) + base::Hours(1);
  base::TimeDelta end_time_delta = base::Days(4) - base::Hours(1);
  uint64_t saving = 10000;

  InitStorage(14);
  state_->AddDelta(saving);

  task_environment_.AdvanceClock(base::Days(1));
  state_->AddDelta(saving);
  state_->AddDelta(saving);

  task_environment_.AdvanceClock(base::Days(2));

  base::Time midnight = Midnight(base::Time::Now());
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight - start_time_delta,
                                            midnight - end_time_delta),
            0U);

  task_environment_.AdvanceClock(base::Days(1));
  midnight = Midnight(base::Time::Now());
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight - start_time_delta,
                                            midnight - end_time_delta),
            saving);

  task_environment_.AdvanceClock(base::Days(1));
  midnight = Midnight(base::Time::Now());
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight - start_time_delta,
                                            midnight - end_time_delta),
            saving * 3);

  task_environment_.AdvanceClock(base::Days(5));
  midnight = Midnight(base::Time::Now());
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight - start_time_delta,
                                            midnight - end_time_delta),
            saving * 2);

  task_environment_.AdvanceClock(base::Days(1));
  midnight = Midnight(base::Time::Now());
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight - start_time_delta,
                                            midnight - end_time_delta),
            0U);
}

TEST_F(SerpMetricsTimePeriodStorageTest, ForgetsOldSavingsWeekly) {
  InitStorage(7);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), saving);

  task_environment_.AdvanceClock(base::Days(8));

  // More savings
  state_->AddDelta(saving);
  state_->AddDelta(saving);
  // Should have forgotten about older days
  EXPECT_EQ(state_->GetPeriodSum(), saving * 2);
}

TEST_F(SerpMetricsTimePeriodStorageTest, ForgetsOldSavingsMonthly) {
  InitStorage(30);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), saving);

  task_environment_.AdvanceClock(base::Days(31));

  // More savings
  state_->AddDelta(saving);
  state_->AddDelta(saving);
  // Should have forgotten about older days
  EXPECT_EQ(state_->GetPeriodSum(), saving * 2);
}

TEST_F(SerpMetricsTimePeriodStorageTest, RetrievesDailySavings) {
  InitStorage(7);
  uint64_t saving = 10000;
  for (int day = 0; day <= 7; day++) {
    task_environment_.AdvanceClock(base::Days(1));
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetPeriodSum(), 7 * saving);
}

TEST_F(SerpMetricsTimePeriodStorageTest, HandlesSkippedDay) {
  InitStorage(7);
  uint64_t saving = 10000;
  for (int day = 0; day < 7; day++) {
    task_environment_.AdvanceClock(base::Days(1));
    if (day == 3) {
      continue;
    }
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetPeriodSum(), 6 * saving);
}

TEST_F(SerpMetricsTimePeriodStorageTest, IntermittentUsageWeekly) {
  InitStorage(7);
  uint64_t saving = 10000;
  for (int day = 0; day < 10; day++) {
    task_environment_.AdvanceClock(base::Days(2));
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetPeriodSum(), 4 * saving);
}

TEST_F(SerpMetricsTimePeriodStorageTest, IntermittentUsageMonthly) {
  InitStorage(30);
  uint64_t saving = 10000;
  for (int day = 0; day < 40; day++) {
    task_environment_.AdvanceClock(base::Days(10));
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetPeriodSum(), 3 * saving);
}

TEST_F(SerpMetricsTimePeriodStorageTest, InfrequentUsageWeekly) {
  InitStorage(7);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  task_environment_.AdvanceClock(base::Days(6));
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), 2 * saving);
}

TEST_F(SerpMetricsTimePeriodStorageTest, InfrequentUsageMonthly) {
  InitStorage(30);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  task_environment_.AdvanceClock(base::Days(29));
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), 2 * saving);
}

TEST_F(SerpMetricsTimePeriodStorageTest, SegregatedListsInDictionary) {
  InitStorage(7, kDictKey1);
  state_->AddDelta(55);

  InitStorage(7, kDictKey2);
  state_->AddDelta(33);

  InitStorage(7, kDictKey1);
  EXPECT_EQ(state_->GetPeriodSum(), 55U);

  InitStorage(7, kDictKey2);
  EXPECT_EQ(state_->GetPeriodSum(), 33U);
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       ValueStoredAtMidnightIsExcludedWhenRangeStartsAfterMidnight) {
  InitStorage(7);
  const uint64_t saving = 10000;
  state_->AddDelta(saving);

  const base::Time midnight = Midnight(base::Time::Now());
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight + base::Minutes(30),
                                            midnight + base::Days(1)),
            0U);
}

}  // namespace serp_metrics
