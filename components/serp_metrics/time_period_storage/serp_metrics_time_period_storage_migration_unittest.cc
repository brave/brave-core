/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/strings/string_util.h"
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
constexpr size_t kPeriodDays = 7;

base::Time FromUTCString(const char* time_string) {
  base::Time time;
  CHECK(base::Time::FromUTCString(time_string, &time));
  return time;
}

struct LocalTimeBucketTimezoneInfo {
  std::string_view timezone_id;
  std::string_view stored_time_periods;
};

// Time period buckets across representative IANA time zones, each storing two
// adjacent local midnight buckets: 2050-01-03 (first day) and 2050-01-04
// (second day).
constexpr LocalTimeBucketTimezoneInfo kLocalTimeBucketTimezones[] = {
    {"Pacific/Pago_Pago",  // UTC−11, no DST.
     R"JSON([
       {  // 2050-01-03 local midnight (UTC Jan 3 11:00)
         "day": 2524820400.0,
         "value": 1.0
       },
       {  // 2050-01-04 local midnight (UTC Jan 4 11:00)
         "day": 2524906800.0,
         "value": 2.0
       }
     ])JSON"},
    {"America/New_York",  // UTC−5/−4, DST, Northern Hemisphere.
     R"JSON([
       {  // 2050-01-03 local midnight (UTC Jan 3 05:00)
         "day": 2524798800.0,
         "value": 1.0
       },
       {  // 2050-01-04 local midnight (UTC Jan 4 05:00)
         "day": 2524885200.0,
         "value": 2.0
       }
     ])JSON"},
    {"America/St_Johns",  // UTC−3:30/−2:30, DST, fractional offset.
     R"JSON([
       {  // 2050-01-03 local midnight (UTC Jan 3 03:30)
         "day": 2524793400.0,
         "value": 1.0
       },
       {  // 2050-01-04 local midnight (UTC Jan 4 03:30)
         "day": 2524879800.0,
         "value": 2.0
       }
     ])JSON"},
    {"UTC",  // UTC±0, no DST.
     R"JSON([
       {  // 2050-01-03 local midnight (UTC Jan 3 00:00)
         "day": 2524780800.0,
         "value": 1.0
       },
       {  // 2050-01-04 local midnight (UTC Jan 4 00:00)
         "day": 2524867200.0,
         "value": 2.0
       }
     ])JSON"},
    {"Europe/Paris",  // UTC+1/+2, DST, Northern Hemisphere.
     R"JSON([
       {  // 2050-01-03 local midnight (UTC Jan 2 23:00)
         "day": 2524777200.0,
         "value": 1.0
       },
       {  // 2050-01-04 local midnight (UTC Jan 3 23:00)
         "day": 2524863600.0,
         "value": 2.0
       }
     ])JSON"},
    {"Asia/Kolkata",  // UTC+5:30, no DST, fractional offset.
     R"JSON([
       {  // 2050-01-03 local midnight (UTC Jan 2 18:30)
         "day": 2524761000.0,
         "value": 1.0
       },
       {  // 2050-01-04 local midnight (UTC Jan 3 18:30)
         "day": 2524847400.0,
         "value": 2.0
       }
     ])JSON"},
    {"Asia/Tokyo",  // UTC+9, no DST, whole-hour offset.
     R"JSON([
       {  // 2050-01-03 local midnight (UTC Jan 2 15:00)
         "day": 2524748400.0,
         "value": 1.0
       },
       {  // 2050-01-04 local midnight (UTC Jan 3 15:00)
         "day": 2524834800.0,
         "value": 2.0
       }
     ])JSON"},
    {"Pacific/Auckland",  // UTC+12/+13, DST, Southern Hemisphere.
     R"JSON([
       {  // 2050-01-03 local midnight (UTC Jan 2 11:00)
         "day": 2524734000.0,
         "value": 1.0
       },
       {  // 2050-01-04 local midnight (UTC Jan 3 11:00)
         "day": 2524820400.0,
         "value": 2.0
       }
     ])JSON"},
};

const base::Time kFirstDayUTCMidnight = FromUTCString("2050-01-03 00:00:00");

const base::Time kSecondDayUTCMidnight = FromUTCString("2050-01-04 00:00:00");

}  // namespace

class SerpMetricsTimePeriodStorageMigrationTest
    : public ::testing::TestWithParam<LocalTimeBucketTimezoneInfo> {
 public:
  SerpMetricsTimePeriodStorageMigrationTest()
      : scoped_timezone_(GetParam().timezone_id) {
    pref_service_.registry()->RegisterDictionaryPref(kFooPrefName);

    base::Time time;
    CHECK(base::Time::FromUTCString("2050-01-04 12:35:56", &time));
    task_environment_.AdvanceClock(time - base::Time::Now());
  }

 protected:
  std::unique_ptr<SerpMetricsTimePeriodStorage> CreateTimePeriodStorage() {
    pref_service_.SetUserPref(
        std::string(kFooPrefName),
        base::DictValue().Set(
            kBarDictKey,
            base::test::ParseJsonList(GetParam().stored_time_periods)));

    return std::make_unique<SerpMetricsTimePeriodStorage>(
        std::make_unique<SerpMetricsPrefTimePeriodStore>(
            &pref_service_, kFooPrefName, kBarDictKey),
        kPeriodDays);
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  const test::ScopedTimezoneForTesting scoped_timezone_;
  TestingPrefServiceSimple pref_service_;
};

TEST_P(SerpMetricsTimePeriodStorageMigrationTest,
       NoBucketCountedInDayBeforeFirstBucket) {
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage();

  EXPECT_EQ(0U, storage->GetCountForTimeRange(
                    kFirstDayUTCMidnight - base::Days(1),
                    kFirstDayUTCMidnight - base::Milliseconds(1)));
}

TEST_P(SerpMetricsTimePeriodStorageMigrationTest,
       NoBucketCountedInDayAfterSecondBucket) {
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage();

  EXPECT_EQ(0U,
            storage->GetCountForTimeRange(
                kSecondDayUTCMidnight + base::Days(1),
                kSecondDayUTCMidnight + base::Days(2) - base::Milliseconds(1)));
}

TEST_P(SerpMetricsTimePeriodStorageMigrationTest,
       OnlyFirstBucketCountedForRangeEndingAtDayBoundary) {
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage();

  EXPECT_EQ(1U, storage->GetCountForTimeRange(
                    kFirstDayUTCMidnight,
                    kSecondDayUTCMidnight - base::Milliseconds(1)));
}

TEST_P(SerpMetricsTimePeriodStorageMigrationTest,
       OnlySecondBucketCountedForRangeStartingAtDayBoundary) {
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage();

  EXPECT_EQ(2U,
            storage->GetCountForTimeRange(
                kSecondDayUTCMidnight,
                kSecondDayUTCMidnight + base::Days(1) - base::Milliseconds(1)));
}

TEST_P(SerpMetricsTimePeriodStorageMigrationTest,
       BothBucketsCountedForRangeStartingAtFirstBucket) {
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage();

  EXPECT_EQ(
      3U, storage->GetCountForTimeRange(kFirstDayUTCMidnight,
                                        kFirstDayUTCMidnight + base::Days(7)));
}

TEST_P(SerpMetricsTimePeriodStorageMigrationTest,
       BothBucketsCountedForRangeEndingAtSecondBucket) {
  const std::unique_ptr<SerpMetricsTimePeriodStorage> storage =
      CreateTimePeriodStorage();

  EXPECT_EQ(3U,
            storage->GetCountForTimeRange(kSecondDayUTCMidnight - base::Days(7),
                                          kSecondDayUTCMidnight));
}

INSTANTIATE_TEST_SUITE_P(
    SerpMetricsTimezones,
    SerpMetricsTimePeriodStorageMigrationTest,
    ::testing::ValuesIn(kLocalTimeBucketTimezones),
    [](const ::testing::TestParamInfo<LocalTimeBucketTimezoneInfo>&
           test_param) {
      std::string name;
      base::ReplaceChars(test_param.param.timezone_id, "/", "_", &name);
      return name;
    });

}  // namespace serp_metrics
