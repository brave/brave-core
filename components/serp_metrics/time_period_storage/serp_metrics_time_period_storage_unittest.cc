/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_storage.h"

#include <memory>
#include <string_view>

#include "base/check.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_pref_time_period_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

namespace {

constexpr std::string_view kPrefName = "baz";
constexpr std::string_view kFooDictKey = "foo";
constexpr std::string_view kBarDictKey = "bar";

}  // namespace

class SerpMetricsTimePeriodStorageTest : public ::testing::Test {
 public:
  SerpMetricsTimePeriodStorageTest() {
    pref_service_.registry()->RegisterDictionaryPref(kPrefName);

    // Advance to a fixed date to avoid DST-related issues. 4 hours before
    // midnight gives tests room to advance forward within the same day.
    base::Time time;
    CHECK(base::Time::FromString("2050-01-04", &time));
    task_environment_.AdvanceClock(Midnight(time) - base::Hours(4) -
                                   base::Time::Now());
  }

 protected:
  void InitStorage(size_t days) {
    storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
        std::make_unique<SerpMetricsPrefTimePeriodStore>(
            &pref_service_, kPrefName, kFooDictKey),
        days);
  }

  void InitStorage(size_t days, std::string_view pref_key) {
    storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
        std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                         kPrefName, pref_key),
        days);
  }

  static base::Time Midnight(base::Time time) { return time.LocalMidnight(); }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<SerpMetricsTimePeriodStorage> storage_;
};

TEST_F(SerpMetricsTimePeriodStorageTest, StartsZero) {
  InitStorage(7);
  EXPECT_EQ(0ULL, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, AccumulatesValues) {
  InitStorage(7);
  storage_->AddCount(10'000);
  EXPECT_EQ(10'000U, storage_->GetCount());

  storage_->AddCount(10'000);
  storage_->AddCount(10'000);
  EXPECT_EQ(30'000U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, SumsValuesInTimeRange) {
  const base::TimeDelta start_offset = base::Days(9) + base::Hours(1);
  const base::TimeDelta end_offset = base::Days(4) - base::Hours(1);

  InitStorage(14);
  storage_->AddCount(10'000);

  task_environment_.AdvanceClock(base::Days(1));
  storage_->AddCount(10'000);
  storage_->AddCount(10'000);

  task_environment_.AdvanceClock(base::Days(2));

  base::Time midnight = Midnight(base::Time::Now());
  EXPECT_EQ(0U, storage_->GetCountForTimeRange(midnight - start_offset,
                                               midnight - end_offset));

  task_environment_.AdvanceClock(base::Days(1));
  midnight = Midnight(base::Time::Now());
  EXPECT_EQ(10'000U, storage_->GetCountForTimeRange(midnight - start_offset,
                                                    midnight - end_offset));

  task_environment_.AdvanceClock(base::Days(1));
  midnight = Midnight(base::Time::Now());
  EXPECT_EQ(30'000U, storage_->GetCountForTimeRange(midnight - start_offset,
                                                    midnight - end_offset));

  task_environment_.AdvanceClock(base::Days(5));
  midnight = Midnight(base::Time::Now());
  EXPECT_EQ(20'000U, storage_->GetCountForTimeRange(midnight - start_offset,
                                                    midnight - end_offset));

  task_environment_.AdvanceClock(base::Days(1));
  midnight = Midnight(base::Time::Now());
  EXPECT_EQ(0U, storage_->GetCountForTimeRange(midnight - start_offset,
                                               midnight - end_offset));
}

TEST_F(SerpMetricsTimePeriodStorageTest, ForgetsOldValuesWeekly) {
  InitStorage(7);
  storage_->AddCount(10'000);
  EXPECT_EQ(10'000U, storage_->GetCount());

  task_environment_.AdvanceClock(base::Days(8));

  storage_->AddCount(10'000);
  storage_->AddCount(10'000);
  EXPECT_EQ(20'000U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, ForgetsOldValuesMonthly) {
  InitStorage(30);
  storage_->AddCount(10'000);
  EXPECT_EQ(10'000U, storage_->GetCount());

  task_environment_.AdvanceClock(base::Days(31));

  storage_->AddCount(10'000);
  storage_->AddCount(10'000);
  EXPECT_EQ(20'000U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, OneDayWindowForgetsPreviousDay) {
  // Arrange
  InitStorage(1);
  storage_->AddCount(10'000);

  // Act
  task_environment_.AdvanceClock(base::Days(1));
  storage_->AddCount(10'000);

  // Assert
  EXPECT_EQ(10'000U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, RetrievesDailyValues) {
  InitStorage(7);
  for (int day = 0; day <= 7; day++) {
    task_environment_.AdvanceClock(base::Days(1));
    storage_->AddCount(10'000);
  }
  EXPECT_EQ(70'000U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, HandlesSkippedDay) {
  InitStorage(7);
  for (int day = 0; day < 7; day++) {
    task_environment_.AdvanceClock(base::Days(1));
    if (day == 3) {
      continue;
    }
    storage_->AddCount(10'000);
  }
  EXPECT_EQ(60'000U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, IntermittentUsageWeekly) {
  InitStorage(7);
  for (int day = 0; day < 10; day++) {
    task_environment_.AdvanceClock(base::Days(2));
    storage_->AddCount(10'000);
  }
  EXPECT_EQ(40'000U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, IntermittentUsageMonthly) {
  InitStorage(30);
  for (int day = 0; day < 40; day++) {
    task_environment_.AdvanceClock(base::Days(10));
    storage_->AddCount(10'000);
  }
  EXPECT_EQ(30'000U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, InfrequentUsageWeekly) {
  InitStorage(7);
  storage_->AddCount(10'000);
  task_environment_.AdvanceClock(base::Days(6));
  storage_->AddCount(10'000);
  EXPECT_EQ(20'000U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, InfrequentUsageMonthly) {
  InitStorage(30);
  storage_->AddCount(10'000);
  task_environment_.AdvanceClock(base::Days(29));
  storage_->AddCount(10'000);
  EXPECT_EQ(20'000U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, SegregatedListsInDictionary) {
  InitStorage(7, kFooDictKey);
  storage_->AddCount(55);

  InitStorage(7, kBarDictKey);
  storage_->AddCount(33);

  InitStorage(7, kFooDictKey);
  EXPECT_EQ(55U, storage_->GetCount());

  InitStorage(7, kBarDictKey);
  EXPECT_EQ(33U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, ClearRemovesStoredData) {
  // Arrange
  InitStorage(7);
  storage_->AddCount(10'000);

  // Act
  storage_->Clear();

  // Assert
  EXPECT_EQ(0U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, AccumulatesAfterClear) {
  // Arrange
  InitStorage(7);
  storage_->AddCount(10'000);
  storage_->Clear();

  // Act
  storage_->AddCount(5'000);

  // Assert
  EXPECT_EQ(5'000U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest, RangeQueryExcludesBucketBeforeStart) {
  // Arrange
  InitStorage(7);
  storage_->AddCount(10'000);
  const base::Time midnight = Midnight(base::Time::Now());

  // Assert
  EXPECT_EQ(0U, storage_->GetCountForTimeRange(midnight + base::Minutes(30),
                                               midnight + base::Days(1)));
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       RangeQueryIncludesBucketAtStartBoundary) {
  // Arrange
  InitStorage(7);
  storage_->AddCount(10'000);
  const base::Time midnight = Midnight(base::Time::Now());

  // Assert
  EXPECT_EQ(10'000U,
            storage_->GetCountForTimeRange(midnight, midnight + base::Days(1)));
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       RangeQueryIncludesBucketAtEndBoundary) {
  // Arrange
  InitStorage(7);
  task_environment_.AdvanceClock(base::Days(1));
  storage_->AddCount(10'000);
  const base::Time midnight_today = Midnight(base::Time::Now());

  // Assert
  EXPECT_EQ(10'000U, storage_->GetCountForTimeRange(
                         midnight_today - base::Days(1), midnight_today));
}

TEST_F(SerpMetricsTimePeriodStorageTest, RangeQueryExcludesBucketAfterEnd) {
  // Arrange
  InitStorage(7);
  task_environment_.AdvanceClock(base::Days(1));
  storage_->AddCount(10'000);
  const base::Time midnight_yesterday =
      Midnight(base::Time::Now()) - base::Days(1);

  // Assert
  EXPECT_EQ(0U, storage_->GetCountForTimeRange(
                    midnight_yesterday, midnight_yesterday + base::Hours(1)));
}

}  // namespace serp_metrics
