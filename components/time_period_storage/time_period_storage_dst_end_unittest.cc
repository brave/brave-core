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

struct DSTEndTimezoneParamInfo {
  std::string_view timezone;
  const char* standard_time_start = nullptr;
  std::string_view expected_json;
};

const auto kDSTEndTimezones = ::testing::Values(
    // America/New_York: EDT -> EST, 1 November 2026 at 06:00 UTC (-4h -> -5h).
    DSTEndTimezoneParamInfo{"America/New_York", "1 Nov 2026 14:00:00",
                            R"JSON([
          {  // November 2 00:00 EST (November 2 05:00 UTC)
            "day": 1793595600.0,
            "value": 1.0
          },
          {  // November 1 00:00 EDT (November 1 04:00 UTC), the DST end day
            "day": 1793505600.0,
            "value": 1.0
          }
        ])JSON"},
    // America/St_Johns: NDT -> NST, 1 November 2026 at 05:30 UTC (-2:30h ->
    // -3:30h). Half-hour base UTC offset.
    DSTEndTimezoneParamInfo{"America/St_Johns", "1 Nov 2026 14:00:00",
                            R"JSON([
          {  // November 2 00:00 NST (November 2 03:30 UTC)
            "day": 1793590200.0,
            "value": 1.0
          },
          {  // November 1 00:00 NDT (November 1 02:30 UTC), the DST end day
            "day": 1793500200.0,
            "value": 1.0
          }
        ])JSON"},
    // Antarctica/Troll: UTC+2 -> UTC+0, 25 October 2026 at 01:00 UTC. Only
    // active timezone with a 2-hour DST shift.
    DSTEndTimezoneParamInfo{"Antarctica/Troll", "25 Oct 2026 11:00:00",
                            R"JSON([
          {  // October 26 00:00 UTC+0 (October 26 00:00 UTC)
            "day": 1792972800.0,
            "value": 1.0
          },
          {  // October 25 00:00 UTC+2 (October 24 22:00 UTC), the DST end day
            "day": 1792879200.0,
            "value": 1.0
          }
        ])JSON"},
    // Europe/Paris: CEST -> CET, 25 October 2026 at 01:00 UTC (+2h -> +1h).
    DSTEndTimezoneParamInfo{"Europe/Paris", "25 Oct 2026 11:00:00",
                            R"JSON([
          {  // October 26 00:00 CET (October 25 23:00 UTC)
            "day": 1792969200.0,
            "value": 1.0
          },
          {  // October 25 00:00 CEST (October 24 22:00 UTC), the DST end day
            "day": 1792879200.0,
            "value": 1.0
          }
        ])JSON"},
    // Australia/Lord_Howe: LHDT -> LHST, 3 April 2027 at 15:00 UTC (+11h ->
    // +10:30h). Only active timezone with a 30-minute DST shift.
    DSTEndTimezoneParamInfo{"Australia/Lord_Howe", "4 Apr 2027 11:00:00",
                            R"JSON([
          {  // April 5 00:00 LHST (April 4 13:30 UTC)
            "day": 1806845400.0,
            "value": 1.0
          },
          {  // April 4 00:00 LHDT (April 3 13:00 UTC), the DST end day
            "day": 1806757200.0,
            "value": 1.0
          }
        ])JSON"},
    // Pacific/Auckland: NZDT -> NZST, 3 April 2027 at 14:00 UTC (+13h ->
    // +12h). Far-east, southern hemisphere DST end in April.
    DSTEndTimezoneParamInfo{"Pacific/Auckland", "3 Apr 2027 20:00:00",
                            R"JSON([
          {  // April 5 00:00 NZST (April 4 12:00 UTC)
            "day": 1806840000.0,
            "value": 1.0
          },
          {  // April 4 00:00 NZDT (April 3 11:00 UTC), the DST end day
            "day": 1806750000.0,
            "value": 1.0
          }
        ])JSON"});

}  // namespace

class TimePeriodStorageDSTEndTest
    : public ::testing::TestWithParam<DSTEndTimezoneParamInfo> {
 protected:
  void FastForwardClockTo(base::Time time) {
    CHECK_GE(time, base::Time::Now());
    task_environment_.AdvanceClock(time - base::Time::Now());
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};

TEST_P(TimePeriodStorageDSTEndTest, CreatesNewDailyBucketAfterDSTEnd) {
  const DSTEndTimezoneParamInfo& timezone_param = GetParam();
  const ScopedTimezoneForTesting scoped_timezone(timezone_param.timezone);

  TestingPrefServiceSimple pref_service;
  pref_service.registry()->RegisterListPref(kPrefName);

  base::Time standard_time_start;
  ASSERT_TRUE(base::Time::FromUTCString(timezone_param.standard_time_start,
                                        &standard_time_start));
  FastForwardClockTo(standard_time_start);

  const auto time_period_storage = std::make_unique<TimePeriodStorage>(
      &pref_service, kPrefName, /*period_days=*/28,
      /*should_offset_dst=*/false);

  // Day 1: the DST end day itself. The local day is longer than 24 hours
  // because clocks fall back.
  time_period_storage->AddDelta(1);

  // Day 2: one full day later. `NextMidnight` must land at the correct next
  // local midnight even though the previous day was longer than 24 hours.
  task_environment_.AdvanceClock(base::Days(1));
  time_period_storage->AddDelta(1);

  // Each call created its own daily bucket. Day 2 is at the front.
  EXPECT_EQ(base::test::ParseJson(timezone_param.expected_json),
            pref_service.GetValue(kPrefName));
}

INSTANTIATE_TEST_SUITE_P(
    DSTEndTimezones,
    TimePeriodStorageDSTEndTest,
    kDSTEndTimezones,
    [](const ::testing::TestParamInfo<DSTEndTimezoneParamInfo>& test_param) {
      std::string name;
      base::ReplaceChars(test_param.param.timezone, "/", "_", &name);
      return name;
    });
