/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics.h"

#include <memory>
#include <string_view>

#include "absl/strings/str_format.h"
#include "base/test/icu_test_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_libc_timezone_override.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/fake_time_period_store.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/serp_metrics/timezone_test_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// Day numbering is relative to the start of the test: Day 0 is the first day,
// Day 1 is the next day, and so on. "Today" always refers to the most recent
// day. Daily usage pings are simulated at specific times to verify which
// metrics are included or excluded based on the `kLastCheckYMD`. Usage pings
// report metrics from yesterday as well as from the stale period.

namespace serp_metrics {

class SerpMetricsTest : public testing::TestWithParam<std::string_view> {
 public:
  SerpMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        libc_timezone_(std::string(GetParam())),
        icu_timezone_(GetParam().data()) {}

  void SetUp() override {
    // Register `kLastCheckYMD` pref (YYYY-MM-DD). This pref is part of the
    // daily usage ping and tracks the last reported day so we don't re-report
    // previously sent metrics.
    local_state_.registry()->RegisterStringPref(kLastCheckYMD,
                                                "");  // Never checked.

    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kSerpMetricsFeature, {{"time_period_in_days", "7"}});

    serp_metrics_ = std::make_unique<SerpMetrics>(
        &local_state_, test::FakeTimePeriodStoreFactory());
  }

  // Advances the clock to one millisecond shy of a brand new day.
  void AdvanceClockToJustBeforeNextDay() {
    const base::Time now = base::Time::Now();
    const base::Time end_of_day =
        now.LocalMidnight() + base::Days(1) - base::Milliseconds(1);
    task_environment_.AdvanceClock(end_of_day - now);
  }

  // Advances the clock to the start of a brand new day.
  void AdvanceClockToNextDay() {
    task_environment_.AdvanceClock(base::Days(1));
  }

  // Advances the clock beyond retention, dropping expired metrics.
  void AdvanceClockByRetentionPeriod() {
    task_environment_.AdvanceClock(
        base::Days(kSerpMetricsTimePeriodInDays.Get()));
  }

  // Simulates sending the daily usage ping by updating `kLastCheckYMD`.
  // Searches are reported by calendar day based on the last checked date.
  void SimulateSendingDailyUsagePingAt(base::Time at) {
    base::Time::Exploded now_exploded;
    at.LocalExplode(&now_exploded);
    local_state_.SetString(
        kLastCheckYMD,
        absl::StrFormat("%04d-%02d-%02d", now_exploded.year, now_exploded.month,
                        now_exploded.day_of_month));
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedLibcTimezoneOverride libc_timezone_;
  const base::test::ScopedRestoreDefaultTimezone icu_timezone_;

  base::test::ScopedFeatureList scoped_feature_list_;

  TestingPrefServiceSimple local_state_;

  std::unique_ptr<SerpMetrics> serp_metrics_;
};

INSTANTIATE_TEST_SUITE_P(SerpMetricsTimezones,
                         SerpMetricsTest,
                         test::kTimezones,
                         test::TimezoneTestParamName);

TEST_P(SerpMetricsTest, NoSearchCountsWhenNoSearchesRecorded) {
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, NoBraveSearchCountForYesterday) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Yesterday (no searches)
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_P(SerpMetricsTest, BraveSearchCountForYesterday) {
  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_P(SerpMetricsTest, NoGoogleSearchCountForYesterday) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 1: Yesterday (no searches)
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
}

TEST_P(SerpMetricsTest, GoogleSearchCountForYesterday) {
  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
}

TEST_P(SerpMetricsTest, NoOtherSearchCountForYesterday) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Yesterday (no searches)
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
}

TEST_P(SerpMetricsTest, OtherSearchCountForYesterday) {
  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
}

TEST_P(SerpMetricsTest, SearchCountForYesterday) {
  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
}

TEST_P(SerpMetricsTest, SearchCountForYesterdayOnCuspOfDayRollover) {
  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  AdvanceClockToJustBeforeNextDay();
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
}

TEST_P(SerpMetricsTest, SearchCountForYesterdayWhenTodayHasNoRecordedSearches) {
  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Today (no searches)

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
}

TEST_P(SerpMetricsTest, DailyUsagePingIncludesYesterdayCounts) {
  // Verifies that yesterday’s searches are included when the last daily usage
  // ping was sent on the previous day.

  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  SimulateSendingDailyUsagePingAt(base::Time::Now());
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
}

TEST_P(SerpMetricsTest, BraveSearchCountForStalePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, GoogleSearchCountForStalePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, OtherSearchCountForStalePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, SearchCountForStalePeriodAcrossMultipleDays) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 1: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 2: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 3: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 4: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(6U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, SearchCountsForYesterdayAndStalePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(4U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, SearchCountForStalePeriodOnCuspOfDayRollover) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  AdvanceClockToJustBeforeNextDay();
  EXPECT_EQ(3U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, DoNotCountSearchesBeforeLastDailyUsagePingWasSent) {
  // Verifies that sending the daily usage ping updates the reporting cutoff
  // and prevents re-reporting older searches.

  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 1: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  const base::Time first_daily_usage_ping_at = base::Time::Now();
  AdvanceClockToNextDay();

  // Day 2: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 3: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  const base::Time second_daily_usage_ping_at = base::Time::Now();
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 4: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // First daily usage ping. Searches from Day 0 have already been reported.
  // Searches from Day 1, Day 2, and yesterday are counted.
  SimulateSendingDailyUsagePingAt(first_daily_usage_ping_at);
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(4U, serp_metrics_->GetSearchCountForStalePeriod());

  // Second daily usage ping. Searches from Day 1 and Day 2 have already been
  // reported. Only yesterday’s searches are counted.
  SimulateSendingDailyUsagePingAt(second_daily_usage_ping_at);
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());

  // Final daily usage ping. Yesterday’s searches are already reported. We are
  // all caught up. Nothing else to include.
  SimulateSendingDailyUsagePingAt(base::Time::Now());
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, CountAllSearchesIfDailyUsagePingWasNeverSent) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 2: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 3: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  local_state_.SetString(kLastCheckYMD, "");  // Never checked.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(3U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, CountAllSearchesIfLastCheckedDateIsInvalid) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 2: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 3: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  local_state_.SetString(kLastCheckYMD, "invalid");
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(3U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, DoNotCountSearchesWhenLastCheckedDateIsInFuture) {
  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  SimulateSendingDailyUsagePingAt(
      base::Time::Now() +
      base::Days(365));  // Time travel for testing purposes only.
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, DoNotCountSearchesOutsideGivenRetentionPeriod) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByRetentionPeriod();

  // Day 7: Stale (day 0 falls outside the retention window)
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 8: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 9: Today (no searches)

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_P(SerpMetricsTest, ClearHistoryClearsAllSearchCounts) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_P(SerpMetricsTest, ClearHistoryDoesNotRestoreClearedSearchCounts) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));

  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_P(SerpMetricsTest, ClearHistoryWithNoSearchesRecorded) {
  // Day 0: Today (no searches)

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_P(SerpMetricsTest, ClearHistoryDoesNotAffectDailyUsagePing) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  SimulateSendingDailyUsagePingAt(base::Time::Now());
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_P(SerpMetricsTest, ClearHistoryIsIdempotent) {
  // Day 0: Today
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  serp_metrics_->ClearHistory();
  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

}  // namespace serp_metrics
