/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_pref_time_period_store.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_storage.h"
#include "brave/components/serp_metrics/time_period_storage/test/scoped_timezone_for_testing.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

namespace {

constexpr std::string_view kFooPrefName = "foo";
constexpr std::string_view kBarDictKey = "bar";
constexpr size_t kPeriodDays = 28;

base::Time FromUTCString(const char* time_string) {
  base::Time time;
  CHECK(base::Time::FromUTCString(time_string, &time));
  return time;
}

}  // namespace

class SerpMetricsTimePeriodStorageMigrationTravelTest : public ::testing::Test {
 public:
  SerpMetricsTimePeriodStorageMigrationTravelTest() {
    pref_service_.registry()->RegisterDictionaryPref(kFooPrefName);
  }

 protected:
  std::unique_ptr<SerpMetricsTimePeriodStorage> CreateTimePeriodStorage(
      std::string_view time_periods) {
    pref_service_.SetUserPref(
        std::string(kFooPrefName),
        base::DictValue().Set(kBarDictKey,
                              base::test::ParseJsonList(time_periods)));

    return std::make_unique<SerpMetricsTimePeriodStorage>(
        std::make_unique<SerpMetricsPrefTimePeriodStore>(
            &pref_service_, kFooPrefName, kBarDictKey),
        kPeriodDays);
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple pref_service_;
};

TEST_F(SerpMetricsTimePeriodStorageMigrationTravelTest,
       ExtremeEastToExtremeWestMigratesLegacyBucketsOneDayBack) {
  base::Time time;
  CHECK(base::Time::FromUTCString("2050-01-04 12:35:56", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());

  const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Baker");
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage(R"JSON([
    {  // 2050-01-03 Pacific/Kiritimati midnight (UTC Jan 2 10:00)
      "day": 2524730400.0,
      "value": 1.0
    },
    {  // 2050-01-04 Pacific/Kiritimati midnight (UTC Jan 3 10:00)
      "day": 2524816800.0,
      "value": 2.0
    }
  ])JSON");

  // Pacific/Kiritimati Jan 3 midnight (UTC Jan 2 10:00) is not a Pacific/Baker
  // midnight, so it passes through migration and falls in UTC Jan 2.
  EXPECT_EQ(1U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-02 00:00:00"),
                FromUTCString("2050-01-03 00:00:00") - base::Milliseconds(1)));
  // Pacific/Kiritimati Jan 4 midnight (UTC Jan 3 10:00) is not a Pacific/Baker
  // midnight, so it passes through migration and falls in UTC Jan 3.
  EXPECT_EQ(2U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-03 00:00:00"),
                FromUTCString("2050-01-04 00:00:00") - base::Milliseconds(1)));
}

TEST_F(SerpMetricsTimePeriodStorageMigrationTravelTest,
       ExtremeWestToExtremeEastMigratesLegacyBucketsOneDayForward) {
  base::Time time;
  CHECK(base::Time::FromUTCString("2050-01-04 12:35:56", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());

  const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Kiritimati");
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage(R"JSON([
    {  // 2050-01-03 Pacific/Baker midnight (UTC Jan 3 12:00)
      "day": 2524824000.0,
      "value": 1.0
    },
    {  // 2050-01-04 Pacific/Baker midnight (UTC Jan 4 12:00)
      "day": 2524910400.0,
      "value": 2.0
    }
  ])JSON");

  // Pacific/Baker Jan 3 midnight (UTC Jan 3 12:00) is not a Pacific/Kiritimati
  // midnight, so it passes through migration and falls in UTC Jan 3.
  EXPECT_EQ(1U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-03 00:00:00"),
                FromUTCString("2050-01-04 00:00:00") - base::Milliseconds(1)));
  // Pacific/Baker Jan 4 midnight (UTC Jan 4 12:00) is not a Pacific/Kiritimati
  // midnight, so it passes through migration and falls in UTC Jan 4.
  EXPECT_EQ(2U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-04 00:00:00"),
                FromUTCString("2050-01-05 00:00:00") - base::Milliseconds(1)));
}

TEST_F(SerpMetricsTimePeriodStorageMigrationTravelTest,
       UTCToExtremeWestMigratesLegacyBucketsCorrectly) {
  base::Time time;
  CHECK(base::Time::FromUTCString("2050-01-04 12:35:56", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());

  const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Baker");
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage(R"JSON([
    {  // 2050-01-03 UTC midnight
      "day": 2524780800.0,
      "value": 1.0
    },
    {  // 2050-01-04 UTC midnight
      "day": 2524867200.0,
      "value": 2.0
    }
  ])JSON");

  EXPECT_EQ(1U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-03 00:00:00"),
                FromUTCString("2050-01-04 00:00:00") - base::Milliseconds(1)));
  EXPECT_EQ(2U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-04 00:00:00"),
                FromUTCString("2050-01-05 00:00:00") - base::Milliseconds(1)));
}

TEST_F(SerpMetricsTimePeriodStorageMigrationTravelTest,
       ExtremeWestToUTCMigratesLegacyBucketsCorrectly) {
  base::Time time;
  CHECK(base::Time::FromUTCString("2050-01-04 12:35:56", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());

  const test::ScopedTimezoneForTesting scoped_timezone("UTC");
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage(R"JSON([
    {  // 2050-01-03 Pacific/Baker midnight (UTC Jan 3 12:00)
      "day": 2524824000.0,
      "value": 1.0
    },
    {  // 2050-01-04 Pacific/Baker midnight (UTC Jan 4 12:00)
      "day": 2524910400.0,
      "value": 2.0
    }
  ])JSON");

  EXPECT_EQ(1U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-03 00:00:00"),
                FromUTCString("2050-01-04 00:00:00") - base::Milliseconds(1)));
  EXPECT_EQ(2U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-04 00:00:00"),
                FromUTCString("2050-01-05 00:00:00") - base::Milliseconds(1)));
}

TEST_F(SerpMetricsTimePeriodStorageMigrationTravelTest,
       UTCToExtremeEastMigratesLegacyBucketsCorrectly) {
  base::Time time;
  CHECK(base::Time::FromUTCString("2050-01-04 12:35:56", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());

  const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Kiritimati");
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage(R"JSON([
    {  // 2050-01-03 UTC midnight
      "day": 2524780800.0,
      "value": 1.0
    },
    {  // 2050-01-04 UTC midnight
      "day": 2524867200.0,
      "value": 2.0
    }
  ])JSON");

  EXPECT_EQ(1U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-03 00:00:00"),
                FromUTCString("2050-01-04 00:00:00") - base::Milliseconds(1)));
  EXPECT_EQ(2U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-04 00:00:00"),
                FromUTCString("2050-01-05 00:00:00") - base::Milliseconds(1)));
}

TEST_F(SerpMetricsTimePeriodStorageMigrationTravelTest,
       ExtremeEastToUTCMigratesLegacyBucketsOneDayBack) {
  base::Time time;
  CHECK(base::Time::FromUTCString("2050-01-04 12:35:56", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());

  const test::ScopedTimezoneForTesting scoped_timezone("UTC");
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage(R"JSON([
    {  // 2050-01-03 Pacific/Kiritimati midnight (UTC Jan 2 10:00)
      "day": 2524730400.0,
      "value": 1.0
    },
    {  // 2050-01-04 Pacific/Kiritimati midnight (UTC Jan 3 10:00)
      "day": 2524816800.0,
      "value": 2.0
    }
  ])JSON");

  // Pacific/Kiritimati Jan 3 midnight (UTC Jan 2 10:00) falls in UTC Jan 2.
  EXPECT_EQ(1U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-02 00:00:00"),
                FromUTCString("2050-01-03 00:00:00") - base::Milliseconds(1)));
  // Pacific/Kiritimati Jan 4 midnight (UTC Jan 3 10:00) falls in UTC Jan 3.
  EXPECT_EQ(2U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-03 00:00:00"),
                FromUTCString("2050-01-04 00:00:00") - base::Milliseconds(1)));
}

TEST_F(SerpMetricsTimePeriodStorageMigrationTravelTest,
       AddCountCreatesNewBucketAtUTCMidnightWhenStoredMidnightsAreBad) {
  const test::ScopedTimezoneForTesting scoped_timezone("Europe/Berlin");
  // Loads the production pref data from the bug report and verifies that
  // `AddCount` after the DST change creates a new bucket at the correct UTC
  // midnight.

  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage(R"JSON([
    // These CEST entries are all 1 hour past local midnight.
    {  // April 9 00:00 CEST + 1h
      "day": 1775689200.0,
      "value": 23.0
    },
    {  // April 8 00:00 CEST + 1h
      "day": 1775602800.0,
      "value": 5.0
    },
    {  // April 7 00:00 CEST + 1h
      "day": 1775516400.0,
      "value": 2.0
    },
    {  // April 6 00:00 CEST + 1h
      "day": 1775430000.0,
      "value": 2.0
    },
    {  // April 5 00:00 CEST + 1h
      "day": 1775343600.0,
      "value": 8.0
    },
    {  // April 4 00:00 CEST + 1h
      "day": 1775257200.0,
      "value": 0.0
    },
    {  // April 3 00:00 CEST + 1h
      "day": 1775170800.0,
      "value": 3.0
    },
    {  // April 2 00:00 CEST + 1h
      "day": 1775084400.0,
      "value": 2.0
    },
    {  // April 1 00:00 CEST + 1h
      "day": 1774998000.0,
      "value": 1.0
    },
    {  // March 31 00:00 CEST + 1h
      "day": 1774911600.0,
      "value": 11.0
    },
    {  // March 30 00:00 CEST + 1h
      "day": 1774825200.0,
      "value": 5.0
    },
    // CET entries below are at the correct midnight.
    {  // March 29 00:00 CET (March 28 23:00 UTC)
      "day": 1774738800.0,
      "value": 22.0
    },
    {  // March 28 00:00 CET (March 27 23:00 UTC)
      "day": 1774652400.0,
      "value": 1.0
    },
    {  // March 27 00:00 CET (March 26 23:00 UTC)
      "day": 1774566000.0,
      "value": 4.0
    },
    {  // March 26 00:00 CET (March 25 23:00 UTC)
      "day": 1774479600.0,
      "value": 4.0
    },
    {  // March 25 00:00 CET (March 24 23:00 UTC)
      "day": 1774393200.0,
      "value": 5.0
    },
    {  // March 24 00:00 CET (March 23 23:00 UTC)
      "day": 1774306800.0,
      "value": 1.0
    },
    {  // March 23 00:00 CET (March 22 23:00 UTC)
      "day": 1774220400.0,
      "value": 1.0
    },
    {  // March 22 00:00 CET (March 21 23:00 UTC)
      "day": 1774134000.0,
      "value": 9.0
    },
    {  // March 21 00:00 CET (March 20 23:00 UTC)
      "day": 1774047600.0,
      "value": 10.0
    },
    {  // March 20 00:00 CET (March 19 23:00 UTC)
      "day": 1773961200.0,
      "value": 11.0
    },
    {  // March 19 00:00 CET (March 18 23:00 UTC)
      "day": 1773874800.0,
      "value": 4.0
    },
    {  // March 18 00:00 CET (March 17 23:00 UTC)
      "day": 1773788400.0,
      "value": 1.0
    },
    {  // March 17 00:00 CET (March 16 23:00 UTC)
      "day": 1773702000.0,
      "value": 0.0
    },
    {  // March 16 00:00 CET (March 15 23:00 UTC)
      "day": 1773615600.0,
      "value": 3.0
    },
    {  // March 15 00:00 CET (March 14 23:00 UTC)
      "day": 1773529200.0,
      "value": 2.0
    },
    {  // March 14 00:00 CET (March 13 23:00 UTC)
      "day": 1773442800.0,
      "value": 2.0
    },
    {  // March 13 00:00 CET (March 12 23:00 UTC)
      "day": 1773356400.0,
      "value": 0.0
    }
  ])JSON");

  // April 10 09:00 CEST (April 10 07:00 UTC).
  base::Time april_10;
  ASSERT_TRUE(base::Time::FromUTCString("10 Apr 2026 07:00:00", &april_10));
  task_environment_.AdvanceClock(april_10 - base::Time::Now());

  storage->AddCount(1);

  EXPECT_EQ(base::test::ParseJsonList(R"JSON([
    {  // April 10 00:00 UTC
        "day": 1775779200.0,
        "value": 1.0
    }, {  // April 9 00:00 UTC
        "day": 1775692800.0,
        "value": 0.0
    }, {  // April 8 00:00 UTC
        "day": 1775606400.0,
        "value": 23.0
    }, {  // April 7 00:00 UTC
        "day": 1775520000.0,
        "value": 5.0
    }, {  // April 6 00:00 UTC
        "day": 1775433600.0,
        "value": 2.0
    }, {  // April 5 00:00 UTC
        "day": 1775347200.0,
        "value": 2.0
    }, {  // April 4 00:00 UTC
        "day": 1775260800.0,
        "value": 8.0
    }, {  // April 3 00:00 UTC
        "day": 1775174400.0,
        "value": 0.0
    }, {  // April 2 00:00 UTC
        "day": 1775088000.0,
        "value": 3.0
    }, {  // April 1 00:00 UTC
        "day": 1775001600.0,
        "value": 2.0
    }, {  // March 31 00:00 UTC
        "day": 1774915200.0,
        "value": 1.0
    }, {  // March 30 00:00 UTC
        "day": 1774828800.0,
        "value": 11.0
    }, {  // March 29 00:00 UTC
        "day": 1774742400.0,
        "value": 27.0
    }, {  // March 28 00:00 UTC
        "day": 1774656000.0,
        "value": 1.0
    }, {  // March 27 00:00 UTC
        "day": 1774569600.0,
        "value": 4.0
    }, {  // March 26 00:00 UTC
        "day": 1774483200.0,
        "value": 4.0
    }, {  // March 25 00:00 UTC
        "day": 1774396800.0,
        "value": 5.0
    }, {  // March 24 00:00 UTC
        "day": 1774310400.0,
        "value": 1.0
    }, {  // March 23 00:00 UTC
        "day": 1774224000.0,
        "value": 1.0
    }, {  // March 22 00:00 UTC
        "day": 1774137600.0,
        "value": 9.0
    }, {  // March 21 00:00 UTC
        "day": 1774051200.0,
        "value": 10.0
    }, {  // March 20 00:00 UTC
        "day": 1773964800.0,
        "value": 11.0
    }, {  // March 19 00:00 UTC
        "day": 1773878400.0,
        "value": 4.0
    }, {  // March 18 00:00 UTC
        "day": 1773792000.0,
        "value": 1.0
    }, {  // March 17 00:00 UTC
        "day": 1773705600.0,
        "value": 0.0
    }, {  // March 16 00:00 UTC
        "day": 1773619200.0,
        "value": 3.0
    }, {  // March 15 00:00 UTC
        "day": 1773532800.0,
        "value": 2.0
    }, {  // March 14 00:00 UTC
        "day": 1773446400.0,
        "value": 2.0
    }
  ])JSON"),
            *pref_service_.GetDict(kFooPrefName).FindList(kBarDictKey));

  // The new April 10 bucket is created at the UTC midnight. March 13 00:00 UTC
  // is dropped because the 28 day window can only keep 28 entries.
  EXPECT_EQ(143U, storage->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageMigrationTravelTest,
       EachDayGetsItsOwnBucketWhenRecordingAcrossDSTStart) {
  // Simulates 28 days of activity starting on March 13 in Munich (CET, UTC+1),
  // then a trip to San Francisco starting on March 22 San Francisco time (PDT,
  // UTC-7), and a return to Munich on March 28, just before the March 29
  // switch to CEST.

  const test::ScopedTimezoneForTesting scoped_munich_timezone("Europe/Berlin");

  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage(R"JSON([])JSON");

  base::Time march_13_cet_midnight;
  ASSERT_TRUE(base::Time::FromUTCString("12 Mar 2026 23:00:00",
                                        &march_13_cet_midnight));
  task_environment_.AdvanceClock(march_13_cet_midnight - base::Time::Now());

  // March 13 through March 22 in Munich.
  for (size_t day = 0; day < 10; ++day) {
    if (day > 0) {
      task_environment_.AdvanceClock(base::Days(1));
    }
    storage->AddCount(1);
  }

  {
    const test::ScopedTimezoneForTesting scoped_los_angeles_timezone(
        "America/Los_Angeles");
    // March 22 through March 26 in San Francisco (local time).
    for (size_t day = 0; day < 5; ++day) {
      task_environment_.AdvanceClock(base::Days(1));
      storage->AddCount(1);
    }
  }

  // March 28 onward back in Munich. The San Francisco scope has ended, so the
  // timezone reverts to Europe/Berlin. The first day is in CET, then Munich
  // switches to CEST on March 29.
  for (size_t day = 0; day < 13; ++day) {
    task_environment_.AdvanceClock(base::Days(1));
    storage->AddCount(1);
  }

  EXPECT_EQ(base::test::ParseJsonList(R"JSON([
    {  // April 8 00:00 UTC
        "day": 1775606400.0,
        "value": 1.0
    }, {  // April 7 00:00 UTC
        "day": 1775520000.0,
        "value": 1.0
    }, {  // April 6 00:00 UTC
        "day": 1775433600.0,
        "value": 1.0
    }, {  // April 5 00:00 UTC
        "day": 1775347200.0,
        "value": 1.0
    }, {  // April 4 00:00 UTC
        "day": 1775260800.0,
        "value": 1.0
    }, {  // April 3 00:00 UTC
        "day": 1775174400.0,
        "value": 1.0
    }, {  // April 2 00:00 UTC
        "day": 1775088000.0,
        "value": 1.0
    }, {  // April 1 00:00 UTC
        "day": 1775001600.0,
        "value": 1.0
    }, {  // March 31 00:00 UTC
        "day": 1774915200.0,
        "value": 1.0
    }, {  // March 30 00:00 UTC
        "day": 1774828800.0,
        "value": 1.0
    }, {  // March 29 00:00 UTC
        "day": 1774742400.0,
        "value": 1.0
    }, {  // March 28 00:00 UTC
        "day": 1774656000.0,
        "value": 1.0
    }, {  // March 27 00:00 UTC
        "day": 1774569600.0,
        "value": 1.0
    }, {  // March 26 00:00 UTC
        "day": 1774483200.0,
        "value": 1.0
    }, {  // March 25 00:00 UTC
        "day": 1774396800.0,
        "value": 1.0
    }, {  // March 24 00:00 UTC
        "day": 1774310400.0,
        "value": 1.0
    }, {  // March 23 00:00 UTC
        "day": 1774224000.0,
        "value": 1.0
    }, {  // March 22 00:00 UTC
        "day": 1774137600.0,
        "value": 1.0
    }, {  // March 21 00:00 UTC
        "day": 1774051200.0,
        "value": 1.0
    }, {  // March 20 00:00 UTC
        "day": 1773964800.0,
        "value": 1.0
    }, {  // March 19 00:00 UTC
        "day": 1773878400.0,
        "value": 1.0
    }, {  // March 18 00:00 UTC
        "day": 1773792000.0,
        "value": 1.0
    }, {  // March 17 00:00 UTC
        "day": 1773705600.0,
        "value": 1.0
    }, {  // March 16 00:00 UTC
        "day": 1773619200.0,
        "value": 1.0
    }, {  // March 15 00:00 UTC
        "day": 1773532800.0,
        "value": 1.0
    }, {  // March 14 00:00 UTC
        "day": 1773446400.0,
        "value": 1.0
    }, {  // March 13 00:00 UTC
        "day": 1773360000.0,
        "value": 1.0
    }, {  // March 12 00:00 UTC
        "day": 1773273600.0,
        "value": 1.0
    }
  ])JSON"),
            *pref_service_.GetDict(kFooPrefName).FindList(kBarDictKey));

  EXPECT_EQ(28U, storage->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageMigrationTravelTest,
       UTCMidnightBucketInUTCTimezoneAreNotMigrated) {
  base::Time time;
  CHECK(base::Time::FromUTCString("2050-01-04 12:35:56", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());

  const test::ScopedTimezoneForTesting scoped_timezone("UTC");
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage(R"JSON([
    {  // 2050-01-04 00:00 UTC, also a local midnight in UTC.
      "day": 2524867200.0,
      "value": 1.0
    },
    {  // 2050-01-03 00:00 UTC, also a local midnight in UTC.
      "day": 2524780800.0,
      "value": 2.0
    }
  ])JSON");

  EXPECT_EQ(1U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-04 00:00:00"),
                FromUTCString("2050-01-05 00:00:00") - base::Milliseconds(1)));
  EXPECT_EQ(2U,
            storage->GetCountForTimeRange(
                FromUTCString("2050-01-03 00:00:00"),
                FromUTCString("2050-01-04 00:00:00") - base::Milliseconds(1)));
}

TEST_F(SerpMetricsTimePeriodStorageMigrationTravelTest,
       BucketsThatMapToTheSameUTCDayAfterMigrationAreMerged) {
  const test::ScopedTimezoneForTesting scoped_timezone("Europe/Berlin");

  base::Time april_1;
  ASSERT_TRUE(base::Time::FromUTCString("1 Apr 2026 12:34:56", &april_1));
  task_environment_.AdvanceClock(april_1 - base::Time::Now());

  // March 30 00:00 CEST + 1h (March 29 23:00 UTC) which is not a local
  // midnight, migrates to March 29 UTC. March 29 00:00 CET (March 28 23:00 UTC)
  // is a local midnight and also migrates to March 29 UTC. Both become the same
  // UTC day and are merged.
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage(R"JSON([
    {  // March 30 01:00 CEST (March 29 23:00 UTC).
      "day": 1774825200.0,
      "value": 5.0
    },
    {  // March 29 00:00 CET (March 28 23:00 UTC).
      "day": 1774738800.0,
      "value": 22.0
    }
  ])JSON");

  EXPECT_EQ(27U,
            storage->GetCountForTimeRange(
                FromUTCString("2026-03-29 00:00:00"),
                FromUTCString("2026-03-30 00:00:00") - base::Milliseconds(1)));
  EXPECT_EQ(0U,
            storage->GetCountForTimeRange(
                FromUTCString("2026-03-30 00:00:00"),
                FromUTCString("2026-03-31 00:00:00") - base::Milliseconds(1)));
}

TEST_F(SerpMetricsTimePeriodStorageMigrationTravelTest,
       MergedBucketDoesNotConsumeExtraCapacitySlot) {
  const test::ScopedTimezoneForTesting scoped_timezone("Europe/Berlin");

  base::Time april_1;
  ASSERT_TRUE(base::Time::FromUTCString("1 Apr 2026 12:34:56", &april_1));
  task_environment_.AdvanceClock(april_1 - base::Time::Now());

  // 4 entries spanning 3 distinct UTC days. Two entries (values 2 and 3) map
  // to March 29 UTC and are merged. With a 3-day period, the freed slot lets
  // March 28 UTC fit rather than being dropped at capacity.
  pref_service_.SetUserPref(
      std::string(kFooPrefName),
      base::DictValue().Set(kBarDictKey, base::test::ParseJsonList(R"JSON([
    {  // March 31 01:00 CEST (March 30 23:00 UTC).
      "day": 1774911600.0,
      "value": 1.0
    },
    {  // March 30 01:00 CEST (March 29 23:00 UTC).
      "day": 1774825200.0,
      "value": 2.0
    },
    {  // March 29 00:00 CET (March 28 23:00 UTC).
      "day": 1774738800.0,
      "value": 3.0
    },
    {  // March 28 00:00 CET (March 27 23:00 UTC).
      "day": 1774652400.0,
      "value": 4.0
    }
  ])JSON")));

  auto storage = std::make_unique<SerpMetricsTimePeriodStorage>(
      std::make_unique<SerpMetricsPrefTimePeriodStore>(
          &pref_service_, kFooPrefName, kBarDictKey),
      /*period_days=*/3U);

  EXPECT_EQ(1U,
            storage->GetCountForTimeRange(
                FromUTCString("2026-03-30 00:00:00"),
                FromUTCString("2026-03-31 00:00:00") - base::Milliseconds(1)));
  EXPECT_EQ(5U,
            storage->GetCountForTimeRange(
                FromUTCString("2026-03-29 00:00:00"),
                FromUTCString("2026-03-30 00:00:00") - base::Milliseconds(1)));
  EXPECT_EQ(4U,
            storage->GetCountForTimeRange(
                FromUTCString("2026-03-28 00:00:00"),
                FromUTCString("2026-03-29 00:00:00") - base::Milliseconds(1)));
}

}  // namespace serp_metrics
