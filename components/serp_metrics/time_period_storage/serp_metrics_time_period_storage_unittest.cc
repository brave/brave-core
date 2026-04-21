/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_storage.h"

#include <memory>

#include "base/check.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_scoped_timezone_for_testing.h"
#include "build/buildflag.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

constexpr char kListPrefName[] = "brave.weekly_test";
constexpr char kDictPrefName[] = "brave.weekly_dict_test";
constexpr char kDictKey1[] = "key1";
constexpr char kDictKey2[] = "key2";

struct SerpMetricsTimePeriodStorageTestParam {
  bool should_use_utc;
  bool should_offset_dst;
};

class SerpMetricsTimePeriodStorageTest
    : public ::testing::TestWithParam<SerpMetricsTimePeriodStorageTestParam> {
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
        &pref_service_, pref_name, dict_key, days, should_use_utc(),
        should_offset_dst());
  }

  bool should_use_utc() const { return GetParam().should_use_utc; }

  bool should_offset_dst() const { return GetParam().should_offset_dst; }

  base::Time Midnight(base::Time time) const {
    return should_use_utc() ? time.UTCMidnight() : time.LocalMidnight();
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<SerpMetricsTimePeriodStorage> state_;
};

TEST_P(SerpMetricsTimePeriodStorageTest, StartsZero) {
  InitStorage(7);
  EXPECT_EQ(state_->GetPeriodSum(), 0ULL);
}

TEST_P(SerpMetricsTimePeriodStorageTest, AddsSavings) {
  InitStorage(7);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), saving);

  // Accumulate
  state_->AddDelta(saving);
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), saving * 3);
}

TEST_P(SerpMetricsTimePeriodStorageTest, SubDelta) {
  InitStorage(7);
  state_->AddDelta(5000);
  task_environment_.AdvanceClock(base::Days(1));
  state_->AddDelta(3000);
  task_environment_.AdvanceClock(base::Days(1));
  state_->AddDelta(1000);
  task_environment_.AdvanceClock(base::Days(1));

  state_->SubDelta(500);
  EXPECT_EQ(state_->GetPeriodSum(), 8500U);
  state_->SubDelta(4000);
  EXPECT_EQ(state_->GetPeriodSum(), 4500U);

  task_environment_.AdvanceClock(base::Days(4));
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

TEST_P(SerpMetricsTimePeriodStorageTest, GetSumInCustomPeriod) {
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
            0u);

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
            0u);
}

TEST_P(SerpMetricsTimePeriodStorageTest, ForgetsOldSavingsWeekly) {
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

TEST_P(SerpMetricsTimePeriodStorageTest, ForgetsOldSavingsMonthly) {
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

TEST_P(SerpMetricsTimePeriodStorageTest, RetrievesDailySavings) {
  InitStorage(7);
  uint64_t saving = 10000;
  for (int day = 0; day <= 7; day++) {
    task_environment_.AdvanceClock(base::Days(1));
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetPeriodSum(), 7 * saving);
}

TEST_P(SerpMetricsTimePeriodStorageTest, HandlesSkippedDay) {
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

TEST_P(SerpMetricsTimePeriodStorageTest, IntermittentUsageWeekly) {
  InitStorage(7);
  uint64_t saving = 10000;
  for (int day = 0; day < 10; day++) {
    task_environment_.AdvanceClock(base::Days(2));
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetPeriodSum(), 4 * saving);
}

TEST_P(SerpMetricsTimePeriodStorageTest, IntermittentUsageMonthly) {
  InitStorage(30);
  uint64_t saving = 10000;
  for (int day = 0; day < 40; day++) {
    task_environment_.AdvanceClock(base::Days(10));
    state_->AddDelta(saving);
  }
  EXPECT_EQ(state_->GetPeriodSum(), 3 * saving);
}

TEST_P(SerpMetricsTimePeriodStorageTest, InfrequentUsageWeekly) {
  InitStorage(7);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  task_environment_.AdvanceClock(base::Days(6));
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), 2 * saving);
}

TEST_P(SerpMetricsTimePeriodStorageTest, InfrequentUsageMonthly) {
  InitStorage(30);
  uint64_t saving = 10000;
  state_->AddDelta(saving);
  task_environment_.AdvanceClock(base::Days(29));
  state_->AddDelta(saving);
  EXPECT_EQ(state_->GetPeriodSum(), 2 * saving);
}

TEST_P(SerpMetricsTimePeriodStorageTest, GetHighestValueInPeriod) {
  InitStorage(7);
  uint64_t lowest_value = 20;
  uint64_t low_value = 50;
  uint64_t high_value = 75;
  state_->AddDelta(low_value);
  task_environment_.AdvanceClock(base::Days(1));
  state_->AddDelta(high_value);
  task_environment_.AdvanceClock(base::Days(1));
  state_->AddDelta(lowest_value);
  EXPECT_EQ(state_->GetHighestValueInPeriod(), high_value);
  task_environment_.AdvanceClock(base::Days(1));
  EXPECT_EQ(state_->GetHighestValueInPeriod(), high_value);
}

TEST_P(SerpMetricsTimePeriodStorageTest, RecordsHigherValueForToday) {
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

TEST_P(SerpMetricsTimePeriodStorageTest,
       GetsHighestValueInWeekFromReplacement) {
  InitStorage(30);
  // Add a low value a couple days after a high value,
  // should return highest day value.
  uint64_t low_value = 50;
  uint64_t high_value = 75;
  state_->ReplaceTodaysValueIfGreater(high_value);
  task_environment_.AdvanceClock(base::Days(2));
  state_->ReplaceTodaysValueIfGreater(low_value);
  EXPECT_EQ(state_->GetHighestValueInPeriod(), high_value);
  // Sanity check disparate days were not replaced
  EXPECT_EQ(state_->GetPeriodSum(), high_value + low_value);
}

TEST_P(SerpMetricsTimePeriodStorageTest, ReplaceIfGreaterForDate) {
  InitStorage(30);

  state_->AddDelta(4);
  task_environment_.AdvanceClock(base::Days(1));
  state_->AddDelta(2);
  task_environment_.AdvanceClock(base::Days(1));
  state_->AddDelta(1);
  task_environment_.AdvanceClock(base::Days(1));

  // should replace
  state_->ReplaceIfGreaterForDate(base::Time::Now() - base::Days(2), 3);
  // should not replace
  state_->ReplaceIfGreaterForDate(base::Time::Now() - base::Days(3), 3);

  EXPECT_EQ(state_->GetPeriodSum(), 8U);

  // should insert new daily value
  state_->ReplaceIfGreaterForDate(base::Time::Now() - base::Days(4), 3);
  EXPECT_EQ(state_->GetPeriodSum(), 11U);

  // should store, but should not be in sum because it's too old
  state_->ReplaceIfGreaterForDate(base::Time::Now() - base::Days(31), 10);
  EXPECT_EQ(state_->GetPeriodSum(), 11U);
}

TEST_P(SerpMetricsTimePeriodStorageTest, SegregatedListsInDictionary) {
  InitStorage(7, kDictKey1);
  state_->AddDelta(55);

  InitStorage(7, kDictKey2);
  state_->AddDelta(33);

  InitStorage(7, kDictKey1);
  EXPECT_EQ(state_->GetPeriodSum(), 55U);

  InitStorage(7, kDictKey2);
  EXPECT_EQ(state_->GetPeriodSum(), 33U);
}

// The DST offset expands the query range by 1 hour, allowing values stored at
// midnight to be counted when querying up to 1 hour after midnight.
TEST_P(SerpMetricsTimePeriodStorageTest, DstOffsetExpandsQueryRange) {
  InitStorage(7);
  const uint64_t saving = 10000;
  state_->AddDelta(saving);

  const bool dst_offset_active = !should_use_utc() && should_offset_dst();
  const uint64_t expected_saving = dst_offset_active ? saving : 0U;

  const base::Time midnight = Midnight(base::Time::Now());
  EXPECT_EQ(state_->GetPeriodSumInTimeRange(midnight + base::Minutes(30),
                                            midnight + base::Days(1)),
            expected_saving);
}

// The test is disabled on Windows because `SerpMetricsScopedTimezoneForTesting`
// relies on `ScopedLibcTimezoneOverride`, which is a no-op for IANA timezone
// identifiers on Windows, causing spurious failures.
#if BUILDFLAG(IS_WIN)
#define MAYBE_GetHighestValueInPeriodExcludesDataOutsideWindowAfterDSTTransition \
  DISABLED_GetHighestValueInPeriodExcludesDataOutsideWindowAfterDSTTransition
#else
#define MAYBE_GetHighestValueInPeriodExcludesDataOutsideWindowAfterDSTTransition \
  GetHighestValueInPeriodExcludesDataOutsideWindowAfterDSTTransition
#endif

TEST_P(
    SerpMetricsTimePeriodStorageTest,
    MAYBE_GetHighestValueInPeriodExcludesDataOutsideWindowAfterDSTTransition) {
  // America/New_York DST starts in 2050 on March 13 (the second Sunday of
  // March). Clocks advance at 02:00 EST (07:00 UTC) to 03:00 EDT. These
  // dates are safely after the fixture's mock-time start of 2050-01-04.
  const test::SerpMetricsScopedTimezoneForTesting scoped_timezone(
      "America/New_York");
  const uint64_t low_value = 50;
  const uint64_t high_value = 75;

  InitStorage(7);

  base::Time time;
  ASSERT_TRUE(base::Time::FromString("March 13 2050 01:30:00", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());
  state_->AddDelta(high_value);

  ASSERT_TRUE(base::Time::FromString("March 19 2050 02:30:00", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());
  state_->AddDelta(low_value);
  // Advance two days so the high-value entry falls outside the seven-day
  // window. Two days are needed because the DST transition shifts the local
  // midnight by one hour — one day is not enough to clear the boundary in
  // local-time mode.
  task_environment_.AdvanceClock(base::Days(2));

  EXPECT_EQ(state_->GetHighestValueInPeriod(), low_value);
}

INSTANTIATE_TEST_SUITE_P(
    ,
    SerpMetricsTimePeriodStorageTest,
    ::testing::Values(
        SerpMetricsTimePeriodStorageTestParam{/*should_use_utc=*/true,
                                              /*should_offset_dst=*/true},
        SerpMetricsTimePeriodStorageTestParam{/*should_use_utc=*/true,
                                              /*should_offset_dst=*/false},
        SerpMetricsTimePeriodStorageTestParam{/*should_use_utc=*/false,
                                              /*should_offset_dst=*/true},
        SerpMetricsTimePeriodStorageTestParam{/*should_use_utc=*/false,
                                              /*should_offset_dst=*/false}),
    [](const ::testing::TestParamInfo<SerpMetricsTimePeriodStorageTestParam>&
           info) {
      return std::string(info.param.should_use_utc ? "UTC" : "LocalTime") +
             (info.param.should_offset_dst ? "WithOffsetDST"
                                           : "WithoutOffsetDST");
    });

}  // namespace serp_metrics
