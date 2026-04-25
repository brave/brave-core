/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/serp_metrics/test/fake_serp_metrics_time_period_store.h"
#include "brave/components/serp_metrics/time_period_storage/test/scoped_timezone_for_testing.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

namespace {

struct DSTStartTimezoneParamInfo {
  std::string_view timezone;
  const char* daylight_time_start = nullptr;
};

const auto kDSTStartTimezones = ::testing::Values(
    // America/New_York: EST -> EDT, 14 March 2027 at 07:00 UTC (-5h -> -4h).
    DSTStartTimezoneParamInfo{"America/New_York", "14 Mar 2027 11:00:00"},
    // America/St_Johns: NST -> NDT, 14 March 2027 at 05:30 UTC (-3:30h ->
    // -2:30h). Half-hour base UTC offset.
    DSTStartTimezoneParamInfo{"America/St_Johns", "14 Mar 2027 11:00:00"},
    // Antarctica/Troll: UTC+0 -> UTC+2, 28 March 2027 at 01:00 UTC. Only active
    // timezone with a 2-hour DST shift.
    DSTStartTimezoneParamInfo{"Antarctica/Troll", "28 Mar 2027 11:00:00"},
    // Europe/Paris: CET -> CEST, 28 March 2027 at 01:00 UTC (+1h -> +2h).
    DSTStartTimezoneParamInfo{"Europe/Paris", "28 Mar 2027 11:00:00"},
    // Australia/Lord_Howe: LHST -> LHDT, 2 October 2027 at 15:30 UTC (+10:30h
    // -> +11h). Only active timezone with a 30-minute DST shift.
    DSTStartTimezoneParamInfo{"Australia/Lord_Howe", "3 Oct 2027 11:00:00"},
    // Pacific/Auckland: NZST -> NZDT, 25 September 2027 at 14:00 UTC (+12h ->
    // +13h). Far-east, southern hemisphere DST start in September.
    DSTStartTimezoneParamInfo{"Pacific/Auckland", "25 Sep 2027 20:00:00"});

}  // namespace

class SerpMetricsDSTStartTest
    : public ::testing::TestWithParam<DSTStartTimezoneParamInfo> {
 public:
  SerpMetricsDSTStartTest() : scoped_timezone_(GetParam().timezone) {}

  void SetUp() override {
    const DSTStartTimezoneParamInfo& timezone_param = GetParam();

    base::Time daylight_time_start;
    ASSERT_TRUE(base::Time::FromUTCString(timezone_param.daylight_time_start,
                                          &daylight_time_start));
    FastForwardClockTo(daylight_time_start);

    local_state_.registry()->RegisterStringPref(kLastCheckYMD, "");
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kSerpMetricsFeature, {{"time_period_in_days", "7"}});
    serp_metrics_ = std::make_unique<SerpMetrics>(
        &local_state_, test::FakeSerpMetricsTimePeriodStoreFactory());
  }

 protected:
  void FastForwardClockTo(base::Time time) {
    CHECK_GE(time, base::Time::Now());
    task_environment_.AdvanceClock(time - base::Time::Now());
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  const test::ScopedTimezoneForTesting scoped_timezone_;
  base::test::ScopedFeatureList scoped_feature_list_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<SerpMetrics> serp_metrics_;
};

TEST_P(SerpMetricsDSTStartTest, YesterdayCountIsOneAfterDSTStart) {
  // Day 1: the DST start day itself. The local day is shorter than 24 hours.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 2: one full day later. `NextMidnight` must skip over the shortened day
  // and land at the correct next local midnight, not one clock-hour early.
  task_environment_.AdvanceClock(base::Days(1));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1 is yesterday from day 2's perspective and has exactly 1 search.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

INSTANTIATE_TEST_SUITE_P(
    DSTStartTimezones,
    SerpMetricsDSTStartTest,
    kDSTStartTimezones,
    [](const ::testing::TestParamInfo<DSTStartTimezoneParamInfo>& test_param) {
      std::string name;
      base::ReplaceChars(test_param.param.timezone, "/", "_", &name);
      return name;
    });

}  // namespace serp_metrics
