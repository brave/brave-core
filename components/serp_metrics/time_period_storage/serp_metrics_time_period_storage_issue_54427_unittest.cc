/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cstddef>
#include <memory>
#include <string_view>

#include "base/check_op.h"
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

constexpr std::string_view kPrefName = "baz";
constexpr std::string_view kFooPrefKey = "foo";
constexpr size_t kPeriodDays = 7;

}  // namespace

// Regression test for https://github.com/brave/brave-browser/issues/54427.
// In timezones that observe daylight saving time, activity from consecutive
// days could be stored in the wrong daily bucket across the DST boundary.
class SerpMetricsTimePeriodStorageIssue54427Test : public ::testing::Test {
 public:
  SerpMetricsTimePeriodStorageIssue54427Test() {
    pref_service_.registry()->RegisterDictionaryPref(kPrefName);
  }

 protected:
  void FastForwardClockTo(base::Time time) {
    CHECK_GE(time, base::Time::Now());
    task_environment_.AdvanceClock(time - base::Time::Now());
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<SerpMetricsTimePeriodStorage> time_period_storage_;
};

TEST_F(SerpMetricsTimePeriodStorageIssue54427Test,
       AddCountCreatesNewBucketAfterDSTStartWhenStoredMidnightsAreBad) {
  // Loads the production pref data from the bug report and verifies that
  // `AddCount` after the DST change creates a new bucket at the correct CEST
  // midnight. In the broken code, all stored CEST timestamps from March 30 to
  // April 9 were already 1 hour late, so computing the next bucket by adding
  // 24 hours produced another late timestamp and prevented a new bucket from
  // being created.
  const test::ScopedTimezoneForTesting scoped_timezone("Europe/Berlin");

  // Pre-populate the pref with the 28 entries from the bug report.
  base::Value list_pref = base::test::ParseJson(R"JSON([
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
  base::DictValue dict;
  dict.Set(kFooPrefKey, std::move(list_pref.GetList()));
  pref_service_.Set(kPrefName, base::Value(std::move(dict)));

  time_period_storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
      std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                       kPrefName, kFooPrefKey),
      /*period_days=*/28);

  // April 10 09:00 CEST (April 10 07:00 UTC).
  base::Time april_10;
  ASSERT_TRUE(base::Time::FromUTCString("10 Apr 2026 07:00:00", &april_10));
  FastForwardClockTo(april_10);

  time_period_storage_->AddCount(1);

  // The new April 10 bucket is created at the correct CEST midnight. Older bad
  // timestamps are unchanged. March 13 is dropped because the 28 day window can
  // only keep 28 entries.
  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // April 10 00:00 CEST (April 9 22:00 UTC)
        "day": 1775772000.0,
        "value": 1.0
      },
      // Older CEST entries remain 1 hour late because they were saved that way.
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
      }
    ])JSON")
                .GetList(),
            *pref_service_.GetDict(kPrefName).FindList(kFooPrefKey));
  EXPECT_EQ(143U, time_period_storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageIssue54427Test,
       EachDayGetsItsOwnBucketWhenRecordingAcrossDSTStart) {
  // Simulates 28 days of activity starting on March 13 in Munich (CET, UTC+1),
  // then a trip to San Francisco starting on March 22 San Francisco time (PDT,
  // UTC-7), and a return to Munich on March 28, just before the March 29
  // switch to CEST. The bug showed up after DST started, when activity could be
  // stored under the previous CET midnight instead of the correct CEST
  // midnight.

  const test::ScopedTimezoneForTesting scoped_munich_timezone("Europe/Berlin");

  time_period_storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
      std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                       kPrefName, kFooPrefKey),
      /*period_days=*/28);

  base::Time march_13_cet_midnight;
  ASSERT_TRUE(base::Time::FromUTCString("12 Mar 2026 23:00:00",
                                        &march_13_cet_midnight));
  FastForwardClockTo(march_13_cet_midnight);

  // March 13 through March 22 in Munich. Local midnight is CET.
  for (size_t day = 0; day < 10; ++day) {
    if (day > 0) {
      task_environment_.AdvanceClock(base::Days(1));
    }
    time_period_storage_->AddCount(1);
  }

  {
    const test::ScopedTimezoneForTesting scoped_los_angeles_timezone(
        "America/Los_Angeles");
    // March 22 through March 26 in San Francisco (local time). Local midnight
    // is PDT.
    for (size_t day = 0; day < 5; ++day) {
      task_environment_.AdvanceClock(base::Days(1));
      time_period_storage_->AddCount(1);
    }
  }

  // March 28 onward back in Munich. The San Francisco scope has ended, so the
  // timezone reverts to Europe/Berlin. The first day is in CET, then Munich
  // switches to CEST on March 29.
  for (size_t day = 0; day < 13; ++day) {
    task_environment_.AdvanceClock(base::Days(1));
    time_period_storage_->AddCount(1);
  }

  // March 22 has two buckets — CET and PDT — because `NextMidnight` from the
  // March 22 CET midnight resolves to the March 22 PDT midnight (same calendar
  // date, 8 hours later). March 27 has value 0 because the return trip crosses
  // midnight in Munich without any activity recorded.
  EXPECT_EQ(base::test::ParseJson(R"JSON([
    {  // April 9 00:00 CEST (April 8 22:00 UTC)
      "day": 1775685600.0,
      "value": 1.0
    },
    {  // April 8 00:00 CEST (April 7 22:00 UTC)
      "day": 1775599200.0,
      "value": 1.0
    },
    {  // April 7 00:00 CEST (April 6 22:00 UTC)
      "day": 1775512800.0,
      "value": 1.0
    },
    {  // April 6 00:00 CEST (April 5 22:00 UTC)
      "day": 1775426400.0,
      "value": 1.0
    },
    {  // April 5 00:00 CEST (April 4 22:00 UTC)
      "day": 1775340000.0,
      "value": 1.0
    },
    {  // April 4 00:00 CEST (April 3 22:00 UTC)
      "day": 1775253600.0,
      "value": 1.0
    },
    {  // April 3 00:00 CEST (April 2 22:00 UTC)
      "day": 1775167200.0,
      "value": 1.0
    },
    {  // April 2 00:00 CEST (April 1 22:00 UTC)
      "day": 1775080800.0,
      "value": 1.0
    },
    {  // April 1 00:00 CEST (March 31 22:00 UTC)
      "day": 1774994400.0,
      "value": 1.0
    },
    {  // March 31 00:00 CEST (March 30 22:00 UTC)
      "day": 1774908000.0,
      "value": 1.0
    },
    {  // March 30 00:00 CEST (March 29 22:00 UTC)
      "day": 1774821600.0,
      "value": 1.0
    },
    {  // March 29 00:00 CET (March 28 23:00 UTC)
      // DST starts later that morning at 02:00 CET.
      "day": 1774738800.0,
      "value": 1.0
    },
    {  // March 28 00:00 CET (March 27 23:00 UTC)
      // First day back in Munich.
      "day": 1774652400.0,
      "value": 1.0
    },
    {  // March 27 00:00 CET (March 26 23:00 UTC)
      // Flying from San Francisco to Munich: no activity recorded.
      "day": 1774566000.0,
      "value": 0.0
    },
    {  // March 26 00:00 PDT (March 26 07:00 UTC)
      "day": 1774508400.0,
      "value": 1.0
    },
    {  // March 25 00:00 PDT (March 25 07:00 UTC)
      "day": 1774422000.0,
      "value": 1.0
    },
    {  // March 24 00:00 PDT (March 24 07:00 UTC)
      "day": 1774335600.0,
      "value": 1.0
    },
    {  // March 23 00:00 PDT (March 23 07:00 UTC)
      "day": 1774249200.0,
      "value": 1.0
    },
    {  // March 22 00:00 PDT (March 22 07:00 UTC)
      // First San Francisco entry. `NextMidnight` from the March 22 CET
      // midnight resolves to the March 22 PDT midnight because they share the
      // same calendar date (8 hours apart).
      "day": 1774162800.0,
      "value": 1.0
    },
    {  // March 22 00:00 CET (March 21 23:00 UTC)
      // Last Munich entry.
      "day": 1774134000.0,
      "value": 1.0
    },
    {  // March 21 00:00 CET (March 20 23:00 UTC)
      "day": 1774047600.0,
      "value": 1.0
    },
    {  // March 20 00:00 CET (March 19 23:00 UTC)
      "day": 1773961200.0,
      "value": 1.0
    },
    {  // March 19 00:00 CET (March 18 23:00 UTC)
      "day": 1773874800.0,
      "value": 1.0
    },
    {  // March 18 00:00 CET (March 17 23:00 UTC)
      "day": 1773788400.0,
      "value": 1.0
    },
    {  // March 17 00:00 CET (March 16 23:00 UTC)
      "day": 1773702000.0,
      "value": 1.0
    },
    {  // March 16 00:00 CET (March 15 23:00 UTC)
      "day": 1773615600.0,
      "value": 1.0
    },
    {  // March 15 00:00 CET (March 14 23:00 UTC)
      "day": 1773529200.0,
      "value": 1.0
    },
    {  // March 14 00:00 CET (March 13 23:00 UTC)
      "day": 1773442800.0,
      "value": 1.0
    }
  ])JSON")
                .GetList(),
            *pref_service_.GetDict(kPrefName).FindList(kFooPrefKey));
  // 28 entries: 27 with value 1 and one March 27 with value 0 (no activity
  // during the return flight). March 13 drops off because the extra March 22
  // PDT bucket pushes the list past 28 entries.
  EXPECT_EQ(27U, time_period_storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageIssue54427Test,
       NewDailyBucketIsCreatedOneSecondAfterMidnightOnDayAfterDSTStart) {
  const test::ScopedTimezoneForTesting scoped_timezone("Europe/Berlin");
  time_period_storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
      std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                       kPrefName, kFooPrefKey),
      kPeriodDays);

  // DST starts in Europe/Berlin at 28 March 2027 01:00:00 UTC. Clocks move
  // forward from 02:00 CET to 03:00 CEST, so the day is 23 hours long.
  base::Time dst_start;
  ASSERT_TRUE(base::Time::FromUTCString("28 Mar 2027 01:00:00", &dst_start));

  // Record one second before the DST transition: March 28 00:00 CET
  // (March 27 23:00 UTC).
  FastForwardClockTo(dst_start - base::Seconds(1));
  time_period_storage_->AddCount(1);

  // Record one second after March 29 midnight in CEST (March 28 22:00:01 UTC).
  // Only 23 hours and 1 second separate the two midnights. A naive +24h
  // calculation would overshoot the next midnight.
  FastForwardClockTo(dst_start + base::Hours(21) + base::Seconds(1));
  time_period_storage_->AddCount(1);

  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // March 29 00:00 CEST (March 28 22:00 UTC)
        "day": 1806271200.0,
        "value": 1.0
      },
      {  // March 28 00:00 CET (March 27 23:00 UTC)
        "day": 1806188400.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *pref_service_.GetDict(kPrefName).FindList(kFooPrefKey));
  EXPECT_EQ(2U, time_period_storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageIssue54427Test,
       NewDailyBucketIsCreatedOneSecondAfterMidnightOnDayAfterTwoHourDSTStart) {
  // Antarctica/Troll switches from UTC+0 to UTC+2 at 01:00 UTC on 28 March
  // 2027. March 28 is therefore only 22 hours long. This is the shortest
  // possible calendar day in any active IANA timezone and is a good stress case
  // for bucket rollover.
  const test::ScopedTimezoneForTesting scoped_timezone("Antarctica/Troll");
  time_period_storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
      std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                       kPrefName, kFooPrefKey),
      kPeriodDays);

  base::Time dst_start;
  ASSERT_TRUE(base::Time::FromUTCString("28 Mar 2027 01:00:00", &dst_start));

  // Record 30 minutes before the DST transition. This is still March 28 00:00
  // in UTC+0, the midnight of the short day.
  FastForwardClockTo(dst_start - base::Minutes(30));
  time_period_storage_->AddCount(1);

  // Record one second after March 29 midnight in UTC+2 (March 28 22:00:01 UTC).
  // Only 22 hours and 1 second separate the two midnights. A naive +24h
  // calculation would overshoot the next midnight.
  FastForwardClockTo(dst_start + base::Hours(21) + base::Seconds(1));
  time_period_storage_->AddCount(1);

  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // March 29 00:00 UTC+2 (March 28 22:00 UTC)
        "day": 1806271200.0,
        "value": 1.0
      },
      {  // March 28 00:00 UTC+0 (March 28 00:00 UTC)
        "day": 1806192000.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *pref_service_.GetDict(kPrefName).FindList(kFooPrefKey));
  EXPECT_EQ(2U, time_period_storage_->GetCount());
}

TEST_F(
    SerpMetricsTimePeriodStorageIssue54427Test,
    NewDailyBucketIsCreatedOneSecondAfterMidnightOnDayAfterThirtyMinuteDSTStart) {
  // Australia/Lord_Howe switches from LHST (UTC+10:30) to LHDT (UTC+11) at
  // 02:00 LHST on 3 October 2027, which is 2 October 2027 15:30 UTC. October 3
  // is therefore only 23 hours and 30 minutes long. This is the only active
  // timezone with a 30-minute DST shift.
  const test::ScopedTimezoneForTesting scoped_timezone("Australia/Lord_Howe");
  time_period_storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
      std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                       kPrefName, kFooPrefKey),
      kPeriodDays);

  base::Time dst_start;
  ASSERT_TRUE(base::Time::FromUTCString("2 Oct 2027 15:30:00", &dst_start));

  // Record 30 minutes before the DST transition. This is still October 3
  // 01:30 LHST, within the midnight of the short day.
  FastForwardClockTo(dst_start - base::Minutes(30));
  time_period_storage_->AddCount(1);

  // Record one second after October 4 midnight in LHDT (October 3 13:00:01
  // UTC). Only 23 hours, 30 minutes, and 1 second separate the two midnights.
  // A naive +24h calculation would overshoot the next midnight by 30 minutes.
  FastForwardClockTo(dst_start + base::Hours(21) + base::Minutes(30) +
                     base::Seconds(1));
  time_period_storage_->AddCount(1);

  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // October 4 00:00 LHDT (October 3 13:00 UTC)
        "day": 1822568400.0,
        "value": 1.0
      },
      {  // October 3 00:00 LHST (October 2 13:30 UTC)
        "day": 1822483800.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *pref_service_.GetDict(kPrefName).FindList(kFooPrefKey));
  EXPECT_EQ(2U, time_period_storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageIssue54427Test,
       NewDailyBucketIsCreatedOneSecondAfterMidnightOnDayAfterDSTEnd) {
  const test::ScopedTimezoneForTesting scoped_timezone("Europe/Berlin");
  time_period_storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
      std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                       kPrefName, kFooPrefKey),
      kPeriodDays);

  // DST ends in Europe/Berlin at 31 October 2027 01:00:00 UTC.
  // Clocks move back from 03:00 CEST to 02:00 CET.
  base::Time dst_end;
  ASSERT_TRUE(base::Time::FromUTCString("31 Oct 2027 01:00:00", &dst_end));

  // Record one second before the DST transition: October 31 00:00 CEST
  // (October 30 22:00 UTC).
  FastForwardClockTo(dst_end - base::Seconds(1));
  time_period_storage_->AddCount(1);

  // Record one second after November 1 midnight in CET
  // (October 31 23:00:01 UTC). Because DST ends, the gap between
  // the two local midnights is 25 hours. A naive +24h calculation
  // would land an hour too early.
  FastForwardClockTo(dst_end + base::Hours(22) + base::Seconds(1));
  time_period_storage_->AddCount(1);

  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // November 1 00:00 CET (October 31 23:00 UTC)
        "day": 1825023600.0,
        "value": 1.0
      },
      {  // October 31 00:00 CEST (October 30 22:00 UTC)
        "day": 1824933600.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *pref_service_.GetDict(kPrefName).FindList(kFooPrefKey));
  EXPECT_EQ(2U, time_period_storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageIssue54427Test,
       EachDayFromCETThroughCESTAndBackToCETHasItsOwnBucket) {
  // In Europe/Berlin, DST starts at 01:00 UTC on 28 March 2027 and ends at
  // 01:00 UTC on 31 October 2027. That covers 217 calendar days.
  const test::ScopedTimezoneForTesting scoped_timezone("Europe/Berlin");
  time_period_storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
      std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                       kPrefName, kFooPrefKey),
      kPeriodDays);

  base::Time dst_start;
  ASSERT_TRUE(base::Time::FromUTCString("28 Mar 2027 01:00:00", &dst_start));
  base::Time dst_end;
  ASSERT_TRUE(base::Time::FromUTCString("31 Oct 2027 01:00:00", &dst_end));

  // Record activity at March 28 midnight in CET, two hours before DST starts.
  FastForwardClockTo(dst_start - base::Hours(2));
  time_period_storage_->AddCount(1);

  // Advance one day at a time through the full CEST period and two days after
  // it. Each local calendar day should get its own bucket with value 1.
  while (base::Time::Now() < dst_end + base::Days(2)) {
    task_environment_.AdvanceClock(base::Days(1));
    time_period_storage_->AddCount(1);
  }

  // The loop ends late on November 2 after 220 daily steps. The 7 day window
  // crosses the DST change on October 31, when local midnight shifts by one
  // hour. Because of that, the gap between those two midnights is 25 hours.
  // GetCount() returns 6 rather than 7: the October 28 CEST bucket is at
  // October 27 22:00 UTC, one hour before the window start at October 27
  // 23:00 UTC (CET midnight minus 6 days). The DST offset is not applied to
  // the window boundary, so that bucket falls outside.
  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // November 3 00:00 CET (November 2 23:00 UTC)
        "day": 1825196400.0,
        "value": 1.0
      },
      {  // November 2 00:00 CET (November 1 23:00 UTC)
        "day": 1825110000.0,
        "value": 1.0
      },
      {  // November 1 00:00 CET (October 31 23:00 UTC)
        // This bucket is 25 hours after the previous one.
        "day": 1825023600.0,
        "value": 1.0
      },
      {  // October 31 00:00 CEST (October 30 22:00 UTC)
        "day": 1824933600.0,
        "value": 1.0
      },
      {  // October 30 00:00 CEST (October 29 22:00 UTC)
        "day": 1824847200.0,
        "value": 1.0
      },
      {  // October 29 00:00 CEST (October 28 22:00 UTC)
        "day": 1824760800.0,
        "value": 1.0
      },
      {  // October 28 00:00 CEST (October 27 22:00 UTC)
        "day": 1824674400.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *pref_service_.GetDict(kPrefName).FindList(kFooPrefKey));
  EXPECT_EQ(6U, time_period_storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageIssue54427Test,
       EachDayInFixedOffsetTimezoneHasItsOwnBucket) {
  // Asia/Tokyo stays on UTC+9 all year, so every local day is exactly 24 hours.
  const test::ScopedTimezoneForTesting scoped_timezone("Asia/Tokyo");
  time_period_storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
      std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                       kPrefName, kFooPrefKey),
      kPeriodDays);

  base::Time january_1;
  ASSERT_TRUE(base::Time::FromUTCString("1 Jan 2027 00:00:00", &january_1));
  FastForwardClockTo(january_1);

  // Record activity for `kPeriodDays + 2` consecutive days and verify that each
  // day's activity lands in its own bucket.
  for (size_t day = 0; day < kPeriodDays + 2; ++day) {
    if (day > 0) {
      task_environment_.AdvanceClock(base::Days(1));
    }
    time_period_storage_->AddCount(1);
  }

  // Recording more than `kPeriodDays` entries pushes the two oldest days out
  // of the rolling window.
  EXPECT_EQ(base::test::ParseJson(R"JSON([
      {  // January 9 00:00 JST (January 8 15:00 UTC)
        "day": 1799420400.0,
        "value": 1.0
      },
      {  // January 8 00:00 JST (January 7 15:00 UTC)
        "day": 1799334000.0,
        "value": 1.0
      },
      {  // January 7 00:00 JST (January 6 15:00 UTC)
        "day": 1799247600.0,
        "value": 1.0
      },
      {  // January 6 00:00 JST (January 5 15:00 UTC)
        "day": 1799161200.0,
        "value": 1.0
      },
      {  // January 5 00:00 JST (January 4 15:00 UTC)
        "day": 1799074800.0,
        "value": 1.0
      },
      {  // January 4 00:00 JST (January 3 15:00 UTC)
        "day": 1798988400.0,
        "value": 1.0
      },
      {  // January 3 00:00 JST (January 2 15:00 UTC)
        "day": 1798902000.0,
        "value": 1.0
      }
    ])JSON")
                .GetList(),
            *pref_service_.GetDict(kPrefName).FindList(kFooPrefKey));
  EXPECT_EQ(7U, time_period_storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageIssue54427Test,
       EastToWestTravelCreatesCorrectDailyBuckets) {
  // Simulates travel from Auckland to Los Angeles, a 20-hour westward shift.
  // Each entry must land in the correct bucket for the local timezone.

  // October 10 04:00 NZDT (October 9 15:00 UTC).
  base::Time october_10_nzdt;
  ASSERT_TRUE(
      base::Time::FromUTCString("9 Oct 2027 15:00:00", &october_10_nzdt));
  FastForwardClockTo(october_10_nzdt);

  const test::ScopedTimezoneForTesting scoped_auckland_timezone(
      "Pacific/Auckland");
  time_period_storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
      std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                       kPrefName, kFooPrefKey),
      kPeriodDays);

  // In Auckland. Bucket is October 10 00:00 NZDT (October 9 11:00 UTC).
  time_period_storage_->AddCount(1);

  {
    const test::ScopedTimezoneForTesting scoped_los_angeles_timezone(
        "America/Los_Angeles");
    // Arriving in Los Angeles after advancing 24 hours. Bucket is October 10
    // 00:00 PDT (October 10 07:00 UTC).
    task_environment_.AdvanceClock(base::Days(1));
    time_period_storage_->AddCount(1);
  }  // Returning to Auckland.

  EXPECT_EQ(2U, time_period_storage_->GetCount());
}

TEST_F(SerpMetricsTimePeriodStorageIssue54427Test,
       WestToEastTravelCreatesCorrectDailyBuckets) {
  // Simulates travel from Los Angeles to Auckland, a 20-hour eastward shift.
  // Each entry must land in the correct bucket for the local timezone.

  // October 9 13:00 PDT (October 9 20:00 UTC).
  base::Time october_9_pdt;
  ASSERT_TRUE(base::Time::FromUTCString("9 Oct 2027 20:00:00", &october_9_pdt));
  FastForwardClockTo(october_9_pdt);

  const test::ScopedTimezoneForTesting scoped_los_angeles_timezone(
      "America/Los_Angeles");
  time_period_storage_ = std::make_unique<SerpMetricsTimePeriodStorage>(
      std::make_unique<SerpMetricsPrefTimePeriodStore>(&pref_service_,
                                                       kPrefName, kFooPrefKey),
      kPeriodDays);

  // In Los Angeles. Bucket is October 9 00:00 PDT (October 9 07:00 UTC).
  time_period_storage_->AddCount(1);

  {
    const test::ScopedTimezoneForTesting scoped_auckland_timezone(
        "Pacific/Auckland");
    // Arriving in Auckland after advancing 24 hours. Bucket is October 11
    // 00:00 NZDT (October 10 11:00 UTC).
    task_environment_.AdvanceClock(base::Days(1));
    time_period_storage_->AddCount(1);
  }  // Returning to Los Angeles.

  EXPECT_EQ(2U, time_period_storage_->GetCount());
}

}  // namespace serp_metrics
