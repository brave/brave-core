/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/time_period_storage/scoped_timezone_for_testing.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr const char* kPrefName = "time_period_storage";

struct DSTStartTimezoneParamInfo {
  std::string_view timezone;
  const char* daylight_time_start = nullptr;
  std::string_view expected_json;
};

const auto kDSTStartTimezones = ::testing::Values(
    // America/New_York: EST -> EDT, 14 March 2027 at 07:00 UTC (-5h -> -4h).
    DSTStartTimezoneParamInfo{"America/New_York", "14 Mar 2027 11:00:00",
                              R"JSON([
          {  // March 15 00:00 EDT (March 15 04:00 UTC)
            "day": 1805083200.0,
            "value": 1.0
          },
          {  // March 14 00:00 EST (March 14 05:00 UTC), DST start day
            "day": 1805000400.0,
            "value": 1.0
          }
        ])JSON"},
    // America/St_Johns: NST -> NDT, 14 March 2027 at 05:30 UTC (-3:30h ->
    // -2:30h). Half-hour base UTC offset.
    DSTStartTimezoneParamInfo{"America/St_Johns", "14 Mar 2027 11:00:00",
                              R"JSON([
          {  // March 15 00:00 NDT (March 15 02:30 UTC)
            "day": 1805077800.0,
            "value": 1.0
          },
          {  // March 14 00:00 NST (March 14 03:30 UTC), DST start day
            "day": 1804995000.0,
            "value": 1.0
          }
        ])JSON"},
    // Antarctica/Troll: UTC+0 -> UTC+2, 28 March 2027 at 01:00 UTC. Only
    // active timezone with a 2-hour DST shift.
    DSTStartTimezoneParamInfo{"Antarctica/Troll", "28 Mar 2027 11:00:00",
                              R"JSON([
          {  // March 29 00:00 UTC+2 (March 28 22:00 UTC)
            "day": 1806271200.0,
            "value": 1.0
          },
          {  // March 28 00:00 UTC+0 (March 28 00:00 UTC), DST start day
            "day": 1806192000.0,
            "value": 1.0
          }
        ])JSON"},
    // Europe/Paris: CET -> CEST, 28 March 2027 at 01:00 UTC (+1h -> +2h).
    DSTStartTimezoneParamInfo{"Europe/Paris", "28 Mar 2027 11:00:00",
                              R"JSON([
          {  // March 29 00:00 CEST (March 28 22:00 UTC)
            "day": 1806271200.0,
            "value": 1.0
          },
          {  // March 28 00:00 CET (March 27 23:00 UTC), DST start day
            "day": 1806188400.0,
            "value": 1.0
          }
        ])JSON"},
    // Australia/Lord_Howe: LHST -> LHDT, 2 October 2027 at 15:30 UTC
    // (+10:30h -> +11h). Only active timezone with a 30-minute DST shift.
    DSTStartTimezoneParamInfo{"Australia/Lord_Howe", "3 Oct 2027 11:00:00",
                              R"JSON([
          {  // October 4 00:00 LHDT (October 3 13:00 UTC)
            "day": 1822568400.0,
            "value": 1.0
          },
          {  // October 3 00:00 LHST (October 2 13:30 UTC), DST start day
            "day": 1822483800.0,
            "value": 1.0
          }
        ])JSON"},
    // Pacific/Auckland: NZST -> NZDT, 25 September 2027 at 14:00 UTC (+12h ->
    // +13h). Far-east, southern hemisphere DST start in September.
    DSTStartTimezoneParamInfo{"Pacific/Auckland", "25 Sep 2027 20:00:00",
                              R"JSON([
          {  // September 27 00:00 NZDT (September 26 11:00 UTC)
            "day": 1821956400.0,
            "value": 1.0
          },
          {  // September 26 00:00 NZST (September 25 12:00 UTC), DST start day
            "day": 1821873600.0,
            "value": 1.0
          }
        ])JSON"});

}  // namespace

class TimePeriodStorageDSTStartTest
    : public ::testing::TestWithParam<DSTStartTimezoneParamInfo> {
 protected:
  void FastForwardClockTo(base::Time time) {
    CHECK_GE(time, base::Time::Now());
    task_environment_.AdvanceClock(time - base::Time::Now());
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};

TEST_P(TimePeriodStorageDSTStartTest, CreatesNewDailyBucketAfterDSTStart) {
  const DSTStartTimezoneParamInfo& timezone_param = GetParam();
  const ScopedTimezoneForTesting scoped_timezone(timezone_param.timezone);

  TestingPrefServiceSimple pref_service;
  pref_service.registry()->RegisterListPref(kPrefName);

  base::Time daylight_time_start;
  ASSERT_TRUE(base::Time::FromUTCString(timezone_param.daylight_time_start,
                                        &daylight_time_start));
  FastForwardClockTo(daylight_time_start);

  const auto time_period_storage = std::make_unique<TimePeriodStorage>(
      &pref_service, kPrefName, /*period_days=*/28,
      /*should_offset_dst=*/false);

  // Day 1: the DST start day itself. The local day is shorter than 24 hours.
  time_period_storage->AddDelta(1);

  // Day 2: one full day later. `NextMidnight` must skip over the shortened day
  // and land at the correct next local midnight, not one clock-hour early.
  task_environment_.AdvanceClock(base::Days(1));
  time_period_storage->AddDelta(1);

  // Each call created its own daily bucket. Day 2 is at the front.
  EXPECT_EQ(base::test::ParseJson(timezone_param.expected_json),
            pref_service.GetValue(kPrefName));
}

INSTANTIATE_TEST_SUITE_P(
    DSTStartTimezones,
    TimePeriodStorageDSTStartTest,
    kDSTStartTimezones,
    [](const ::testing::TestParamInfo<DSTStartTimezoneParamInfo>& test_param) {
      std::string name;
      base::ReplaceChars(test_param.param.timezone, "/", "_", &name);
      return name;
    });
