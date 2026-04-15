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
#include "brave/components/time_period_storage/scoped_timezone_for_testing.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "brave/components/time_period_storage/time_period_store.h"
#include "brave/components/time_period_storage/time_period_store_factory.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

namespace {

struct DSTEndTimezoneParamInfo {
  std::string_view timezone;
  const char* standard_time_start = nullptr;
};

const auto kDSTEndTimezones = ::testing::Values(
    // America/New_York: EDT -> EST, 1 November 2026 at 06:00 UTC (-4h -> -5h).
    DSTEndTimezoneParamInfo{"America/New_York", "1 Nov 2026 14:00:00"},
    // America/St_Johns: NDT -> NST, 1 November 2026 at 05:30 UTC (-2:30h ->
    // -3:30h). Half-hour base UTC offset.
    DSTEndTimezoneParamInfo{"America/St_Johns", "1 Nov 2026 14:00:00"},
    // Antarctica/Troll: UTC+2 -> UTC+0, 25 October 2026 at 01:00 UTC. Only
    // active timezone with a 2-hour DST shift.
    DSTEndTimezoneParamInfo{"Antarctica/Troll", "25 Oct 2026 11:00:00"},
    // Europe/Paris: CEST -> CET, 25 October 2026 at 01:00 UTC (+2h -> +1h).
    DSTEndTimezoneParamInfo{"Europe/Paris", "25 Oct 2026 11:00:00"},
    // Australia/Lord_Howe: LHDT -> LHST, 3 April 2027 at 15:00 UTC (+11h ->
    // +10:30h). Only active timezone with a 30-minute DST shift.
    DSTEndTimezoneParamInfo{"Australia/Lord_Howe", "4 Apr 2027 11:00:00"},
    // Pacific/Auckland: NZDT -> NZST, 3 April 2027 at 14:00 UTC (+13h -> +12h).
    // Far-east, southern hemisphere DST end in April.
    DSTEndTimezoneParamInfo{"Pacific/Auckland", "3 Apr 2027 20:00:00"});

class FakeTimePeriodStore : public TimePeriodStore {
 public:
  FakeTimePeriodStore() = default;

  FakeTimePeriodStore(const FakeTimePeriodStore&) = delete;
  FakeTimePeriodStore& operator=(const FakeTimePeriodStore&) = delete;

  ~FakeTimePeriodStore() override = default;

  const base::ListValue* Get() override { return &list_; }

  void Set(base::ListValue list) override { list_ = std::move(list); }

  void Clear() override { list_.clear(); }

 private:
  base::ListValue list_;
};

class FakeTimePeriodStoreFactory : public TimePeriodStoreFactory {
 public:
  FakeTimePeriodStoreFactory() = default;

  FakeTimePeriodStoreFactory(const FakeTimePeriodStoreFactory&) = delete;
  FakeTimePeriodStoreFactory& operator=(const FakeTimePeriodStoreFactory&) =
      delete;

  ~FakeTimePeriodStoreFactory() override = default;

  std::unique_ptr<TimePeriodStore> Build(
      const char* metric_name) const override {
    return std::make_unique<FakeTimePeriodStore>();
  }
};

}  // namespace

class SerpMetricsDSTEndTest
    : public ::testing::TestWithParam<DSTEndTimezoneParamInfo> {
 public:
  SerpMetricsDSTEndTest() : scoped_timezone_(GetParam().timezone) {}

  void SetUp() override {
    const DSTEndTimezoneParamInfo& timezone_param = GetParam();

    base::Time standard_time_start;
    ASSERT_TRUE(base::Time::FromUTCString(timezone_param.standard_time_start,
                                          &standard_time_start));
    FastForwardClockTo(standard_time_start);

    local_state_.registry()->RegisterStringPref(kLastCheckYMD, "");
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kSerpMetricsFeature, {{"time_period_in_days", "7"}});
    serp_metrics_ = std::make_unique<SerpMetrics>(&local_state_,
                                                  FakeTimePeriodStoreFactory());
  }

 protected:
  void FastForwardClockTo(base::Time time) {
    CHECK_GE(time, base::Time::Now());
    task_environment_.AdvanceClock(time - base::Time::Now());
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  const ScopedTimezoneForTesting scoped_timezone_;
  base::test::ScopedFeatureList scoped_feature_list_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<SerpMetrics> serp_metrics_;
};

TEST_P(SerpMetricsDSTEndTest, YesterdayCountIsOneAfterDSTEnd) {
  // Day 1: the DST end day itself. The local day is longer than 24 hours
  // because clocks fall back.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 2: one full day later. `NextMidnight` must land at the correct next
  // local midnight even though the previous day was longer than 24 hours.
  task_environment_.AdvanceClock(base::Days(1));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1 is yesterday from day 2's perspective and has exactly 1 search.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

INSTANTIATE_TEST_SUITE_P(
    DSTEndTimezones,
    SerpMetricsDSTEndTest,
    kDSTEndTimezones,
    [](const ::testing::TestParamInfo<DSTEndTimezoneParamInfo>& test_param) {
      std::string name;
      base::ReplaceChars(test_param.param.timezone, "/", "_", &name);
      return name;
    });

}  // namespace serp_metrics
