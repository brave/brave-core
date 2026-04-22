/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_storage.h"

#include <memory>
#include <string_view>

#include "base/check.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
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

    // Advance to January 4, 2050 UTC so tests have a predictable starting
    // point.
    base::Time time;
    CHECK(base::Time::FromUTCString("2050-01-04", &time));
    task_environment_.AdvanceClock(time.UTCMidnight() - base::Time::Now());
  }

 protected:
  void InitStorage(size_t days) {
    storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
        std::make_unique<SerpMetricsPrefTimePeriodStore>(
            &pref_service_, kPrefName, kFooDictKey),
        days);
  }

  void AdvanceClockToNextUTCMidnight() {
    const base::Time now = base::Time::Now();
    task_environment_.AdvanceClock(now.UTCMidnight() + base::Days(1) - now);
  }

  void AdvanceClockToJustBeforeNextUTCMidnight() {
    const base::Time now = base::Time::Now();
    const base::Time end_of_day =
        now.UTCMidnight() + base::Days(1) - base::Milliseconds(1);
    task_environment_.AdvanceClock(end_of_day - now);
  }

  void InitStorage(size_t days, std::string_view pref_key) {
    storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
        std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                         kPrefName, pref_key),
        days);
  }

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

  base::Time midnight = base::Time::Now().UTCMidnight();
  EXPECT_EQ(0U, storage_->GetCountForTimeRange(midnight - start_offset,
                                               midnight - end_offset));

  task_environment_.AdvanceClock(base::Days(1));
  midnight = base::Time::Now().UTCMidnight();
  EXPECT_EQ(10'000U, storage_->GetCountForTimeRange(midnight - start_offset,
                                                    midnight - end_offset));

  task_environment_.AdvanceClock(base::Days(1));
  midnight = base::Time::Now().UTCMidnight();
  EXPECT_EQ(30'000U, storage_->GetCountForTimeRange(midnight - start_offset,
                                                    midnight - end_offset));

  task_environment_.AdvanceClock(base::Days(5));
  midnight = base::Time::Now().UTCMidnight();
  EXPECT_EQ(20'000U, storage_->GetCountForTimeRange(midnight - start_offset,
                                                    midnight - end_offset));

  task_environment_.AdvanceClock(base::Days(1));
  midnight = base::Time::Now().UTCMidnight();
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
  const base::Time midnight = base::Time::Now().UTCMidnight();

  // Assert
  EXPECT_EQ(0U, storage_->GetCountForTimeRange(midnight + base::Minutes(30),
                                               midnight + base::Days(1)));
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       RangeQueryIncludesBucketAtStartBoundary) {
  // Arrange
  InitStorage(7);
  storage_->AddCount(10'000);
  const base::Time midnight = base::Time::Now().UTCMidnight();

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
  const base::Time midnight_today = base::Time::Now().UTCMidnight();

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
      base::Time::Now().UTCMidnight() - base::Days(1);

  // Assert
  EXPECT_EQ(0U, storage_->GetCountForTimeRange(
                    midnight_yesterday, midnight_yesterday + base::Hours(1)));
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       ActivityJustBeforeMidnightIsInCurrentUTCDayBucket) {
  // Arrange
  AdvanceClockToJustBeforeNextUTCMidnight();
  InitStorage(7);

  // Act
  storage_->AddCount(1);

  // Assert
  const base::ListValue* const list =
      pref_service_.GetDict(kPrefName).FindList(kFooDictKey);
  ASSERT_TRUE(list);
  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // January 4 00:00:00 UTC
        "day": 2524867200.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *list);
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       ActivityAtExactMidnightIsInNewUTCDayBucket) {
  // Arrange
  AdvanceClockToNextUTCMidnight();
  InitStorage(7);

  // Act
  storage_->AddCount(1);

  // Assert
  const base::ListValue* const list =
      pref_service_.GetDict(kPrefName).FindList(kFooDictKey);
  ASSERT_TRUE(list);
  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // January 5 00:00:00 UTC
        "day": 2524953600.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *list);
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       ActivityJustAfterMidnightIsInNewUTCDayBucket) {
  // Arrange
  AdvanceClockToJustBeforeNextUTCMidnight();
  InitStorage(7);
  storage_->AddCount(1);

  // Act
  task_environment_.AdvanceClock(base::Milliseconds(2));
  storage_->AddCount(1);

  // Assert
  const base::ListValue* const list =
      pref_service_.GetDict(kPrefName).FindList(kFooDictKey);
  ASSERT_TRUE(list);
  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // January 5 00:00:00 UTC
        "day": 2524953600.0,
        "value": 1.0
      },
      {  // January 4 00:00:00 UTC
        "day": 2524867200.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *list);
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       ActivityAtMiddayIsInCurrentUTCDayBucket) {
  // Arrange
  task_environment_.AdvanceClock(base::Hours(16));
  InitStorage(7);

  // Act
  storage_->AddCount(1);

  // Assert
  const base::ListValue* const list =
      pref_service_.GetDict(kPrefName).FindList(kFooDictKey);
  ASSERT_TRUE(list);
  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // January 4 00:00:00 UTC
        "day": 2524867200.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *list);
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       FilledBucketsForSkippedDaysAreAtConsecutiveUTCMidnights) {
  // Arrange
  InitStorage(7);
  storage_->AddCount(1);

  // Act
  task_environment_.AdvanceClock(base::Days(4));
  storage_->AddCount(1);

  // Assert
  const base::ListValue* const list =
      pref_service_.GetDict(kPrefName).FindList(kFooDictKey);
  ASSERT_TRUE(list);
  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // January 8 00:00:00 UTC
        "day": 2525212800.0,
        "value": 1.0
      },
      {  // January 7 00:00:00 UTC
        "day": 2525126400.0,
        "value": 0.0
      },
      {  // January 6 00:00:00 UTC
        "day": 2525040000.0,
        "value": 0.0
      },
      {  // January 5 00:00:00 UTC
        "day": 2524953600.0,
        "value": 0.0
      },
      {  // January 4 00:00:00 UTC
        "day": 2524867200.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *list);
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       GetCountExcludesBucketAfterPeriodDaysHaveElapsed) {
  InitStorage(7);
  storage_->AddCount(10'000);

  task_environment_.AdvanceClock(base::Days(7));
  EXPECT_EQ(0U, storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       PersistsAcrossReloadAndCreatesBucketOnNextUTCDay) {
  // Arrange
  InitStorage(7);
  storage_->AddCount(1);
  AdvanceClockToNextUTCMidnight();
  task_environment_.AdvanceClock(base::Hours(6));
  InitStorage(7);

  // Act
  storage_->AddCount(1);

  // Assert
  const base::ListValue* const list =
      pref_service_.GetDict(kPrefName).FindList(kFooDictKey);
  ASSERT_TRUE(list);
  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // January 5 00:00:00 UTC
        "day": 2524953600.0,
        "value": 1.0
      },
      {  // January 4 00:00:00 UTC
        "day": 2524867200.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *list);
}

TEST_F(SerpMetricsTimePeriodStorageTest, LeapDayGetsSeparateUTCMidnightBucket) {
  // 2052 is a leap year. February 29 must produce a distinct bucket at its
  // own UTC midnight, not be skipped or merged with February 28 or March 1.
  base::Time feb_28_2052;
  ASSERT_TRUE(base::Time::FromUTCString("28 Feb 2052 20:00:00", &feb_28_2052));
  task_environment_.AdvanceClock(feb_28_2052 - base::Time::Now());

  InitStorage(7);
  storage_->AddCount(1);

  task_environment_.AdvanceClock(base::Days(1));
  storage_->AddCount(1);

  task_environment_.AdvanceClock(base::Days(1));
  storage_->AddCount(1);

  const base::ListValue* const list =
      pref_service_.GetDict(kPrefName).FindList(kFooDictKey);
  ASSERT_TRUE(list);
  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // March 1 00:00:00 UTC
        "day": 2592864000.0,
        "value": 1.0
      },
      {  // February 29 00:00:00 UTC
        "day": 2592777600.0,
        "value": 1.0
      },
      {  // February 28 00:00:00 UTC
        "day": 2592691200.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *list);
}

TEST_F(SerpMetricsTimePeriodStorageTest,
       SearchesAfterLegacyLocalMidnightBucketAreSegregatedByUTCDay) {
  // Arrange: seed a legacy offset bucket for Jan 3 (value 5).
  base::DictValue legacy_pref;
  legacy_pref.Set(kFooDictKey,
                  base::test::ParseJson(
                      R"JSON([{"day": 2524852800.0, "value": 5.0}])JSON"));
  pref_service_.SetUserPref(std::string(kPrefName), std::move(legacy_pref));
  InitStorage(7);

  // Act: record searches at Jan 4 00:00 UTC.
  storage_->AddCount(3);

  // Assert: UTC-day range queries return the correct count for each day.
  const base::Time jan_3_midnight =
      base::Time::FromSecondsSinceUnixEpoch(2524780800.0);
  const base::Time jan_4_midnight =
      base::Time::FromSecondsSinceUnixEpoch(2524867200.0);
  const base::Time jan_5_midnight =
      base::Time::FromSecondsSinceUnixEpoch(2524953600.0);

  EXPECT_EQ(5U, storage_->GetCountForTimeRange(
                    jan_3_midnight, jan_4_midnight - base::Milliseconds(1)));
  EXPECT_EQ(3U, storage_->GetCountForTimeRange(
                    jan_4_midnight, jan_5_midnight - base::Milliseconds(1)));
}

}  // namespace serp_metrics
