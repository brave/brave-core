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

struct LegacyBucketTimezoneInfo {
  const char* timezone_id;
  // JSON for a bucket whose `day` is local midnight of 2050-01-04 in this
  // timezone. After migration in `SerpMetricsTimePeriodStorage::Load()`, every
  // entry must resolve to the same UTC day: 2050-01-04 00:00:00 UTC =
  // 2524867200 s.
  std::string_view legacy_json;
};

// Timezones ordered from furthest west (UTC-12) to furthest east (UTC+14).
constexpr LegacyBucketTimezoneInfo kLegacyBucketTimezones[] = {
    {"Etc/GMT+12",
     R"JSON([{"day": 2524910400.0, "value": 42.0}])JSON"},  // UTC Jan 4 12:00
    {"America/Los_Angeles",
     R"JSON([{"day": 2524896000.0, "value": 42.0}])JSON"},  // UTC Jan 4 08:00
    {"America/New_York",
     R"JSON([{"day": 2524885200.0, "value": 42.0}])JSON"},  // UTC Jan 4 05:00
    {"UTC",
     R"JSON([{"day": 2524867200.0, "value": 42.0}])JSON"},  // UTC Jan 4 00:00
    {"Europe/Paris",
     R"JSON([{"day": 2524863600.0, "value": 42.0}])JSON"},  // UTC Jan 3 23:00
    {"Asia/Kolkata",
     R"JSON([{"day": 2524847400.0, "value": 42.0}])JSON"},  // UTC Jan 3 18:30
    {"Asia/Shanghai",
     R"JSON([{"day": 2524838400.0, "value": 42.0}])JSON"},  // UTC Jan 3 16:00
    {"Pacific/Auckland",
     R"JSON([{"day": 2524820400.0, "value": 42.0}])JSON"},  // UTC Jan 3 11:00
    {"Pacific/Kiritimati",
     R"JSON([{"day": 2524816800.0, "value": 42.0}])JSON"},  // UTC Jan 3 10:00
};

// The UTC midnight all legacy local midnight buckets for 2050-01-04 migrate to
// after `SerpMetricsTimePeriodStorage::Load()` maps the stored local timestamp
// to its UTC calendar day.
constexpr double kBucketEpochSeconds = 2524867200.0;  // 2050-01-04 00:00 UTC

}  // namespace

class SerpMetricsTimePeriodStorageLegacyBucketTest
    : public ::testing::TestWithParam<LegacyBucketTimezoneInfo> {
 public:
  SerpMetricsTimePeriodStorageLegacyBucketTest() {
    pref_service_.registry()->RegisterDictionaryPref(kFooPrefName);
    base::Time time;
    CHECK(base::Time::FromUTCString("2050-01-04", &time));
    task_environment_.AdvanceClock(time.UTCMidnight() - base::Time::Now());
  }

 protected:
  std::unique_ptr<SerpMetricsTimePeriodStorage> CreateStorage() {
    base::DictValue legacy_pref;
    legacy_pref.Set(kBarDictKey, base::test::ParseJson(GetParam().legacy_json));
    pref_service_.SetUserPref(std::string(kFooPrefName),
                              std::move(legacy_pref));
    return std::make_unique<SerpMetricsTimePeriodStorage>(
        std::make_unique<SerpMetricsPrefTimePeriodStore>(
            &pref_service_, kFooPrefName, kBarDictKey),
        7);
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple pref_service_;
};

TEST_P(SerpMetricsTimePeriodStorageLegacyBucketTest,
       LocalMidnightBucketIsAttributedToCorrectUTCDay) {
  const test::ScopedTimezoneForTesting scoped_timezone(GetParam().timezone_id);
  const auto storage = CreateStorage();

  const base::Time bucket_time =
      base::Time::FromSecondsSinceUnixEpoch(kBucketEpochSeconds);
  EXPECT_EQ(42U, storage->GetCountForTimeRange(
                     bucket_time,
                     bucket_time + base::Days(1) - base::Milliseconds(1)));
}

TEST_P(SerpMetricsTimePeriodStorageLegacyBucketTest,
       BucketIsNotCountedInPrecedingUTCDay) {
  const test::ScopedTimezoneForTesting scoped_timezone(GetParam().timezone_id);
  const auto storage = CreateStorage();

  const base::Time bucket_time =
      base::Time::FromSecondsSinceUnixEpoch(kBucketEpochSeconds);
  EXPECT_EQ(0U,
            storage->GetCountForTimeRange(bucket_time - base::Days(1),
                                          bucket_time - base::Milliseconds(1)));
}

TEST_P(SerpMetricsTimePeriodStorageLegacyBucketTest,
       BucketIsNotCountedInFollowingUTCDay) {
  const test::ScopedTimezoneForTesting scoped_timezone(GetParam().timezone_id);
  const auto storage = CreateStorage();

  const base::Time bucket_time =
      base::Time::FromSecondsSinceUnixEpoch(kBucketEpochSeconds);
  EXPECT_EQ(0U, storage->GetCountForTimeRange(
                    bucket_time + base::Days(1),
                    bucket_time + base::Days(2) - base::Milliseconds(1)));
}

TEST_P(SerpMetricsTimePeriodStorageLegacyBucketTest,
       RangeEndingOneMillisecondBeforeBucketTimestampExcludesBucket) {
  const test::ScopedTimezoneForTesting scoped_timezone(GetParam().timezone_id);
  const auto storage = CreateStorage();

  const base::Time bucket_time =
      base::Time::FromSecondsSinceUnixEpoch(kBucketEpochSeconds);
  EXPECT_EQ(0U,
            storage->GetCountForTimeRange(bucket_time - base::Days(7),
                                          bucket_time - base::Milliseconds(1)));
}

TEST_P(SerpMetricsTimePeriodStorageLegacyBucketTest,
       RangeStartingAtBucketTimestampIncludesBucket) {
  const test::ScopedTimezoneForTesting scoped_timezone(GetParam().timezone_id);
  const auto storage = CreateStorage();

  const base::Time bucket_time =
      base::Time::FromSecondsSinceUnixEpoch(kBucketEpochSeconds);
  EXPECT_EQ(42U, storage->GetCountForTimeRange(bucket_time,
                                               bucket_time + base::Days(7)));
}

TEST_P(SerpMetricsTimePeriodStorageLegacyBucketTest,
       RangeEndingAtBucketTimestampIncludesBucket) {
  const test::ScopedTimezoneForTesting scoped_timezone(GetParam().timezone_id);
  const auto storage = CreateStorage();

  const base::Time bucket_time =
      base::Time::FromSecondsSinceUnixEpoch(kBucketEpochSeconds);
  EXPECT_EQ(42U, storage->GetCountForTimeRange(bucket_time - base::Days(7),
                                               bucket_time));
}

TEST_P(SerpMetricsTimePeriodStorageLegacyBucketTest,
       RangeStartingOneMillisecondAfterBucketTimestampExcludesBucket) {
  const test::ScopedTimezoneForTesting scoped_timezone(GetParam().timezone_id);
  const auto storage = CreateStorage();

  const base::Time bucket_time =
      base::Time::FromSecondsSinceUnixEpoch(kBucketEpochSeconds);
  EXPECT_EQ(0U,
            storage->GetCountForTimeRange(bucket_time + base::Milliseconds(1),
                                          bucket_time + base::Days(7)));
}

INSTANTIATE_TEST_SUITE_P(
    FromWestToEast,
    SerpMetricsTimePeriodStorageLegacyBucketTest,
    ::testing::ValuesIn(kLegacyBucketTimezones),
    [](const ::testing::TestParamInfo<LegacyBucketTimezoneInfo>& info) {
      std::string name = info.param.timezone_id;
      for (char& character : name) {
        if (character == '/' || character == '+' || character == '-') {
          character = '_';
        }
      }
      return name;
    });

}  // namespace serp_metrics
