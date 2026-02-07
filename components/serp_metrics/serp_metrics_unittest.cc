/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics.h"

#include <memory>

#include "absl/strings/str_format.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/pref_registry.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// Day numbering is relative to the start of the test: Day 0 is the first day,
// Day 1 is the next day, and so on. "Today" always refers to the most recent
// day. Daily usage pings are simulated at specific times to verify which
// metrics are included or excluded based on the `kLastCheckYMD`. Usage pings
// report metrics from yesterday as well as from the stale period.

namespace serp_metrics {

class SerpMetricsTest : public testing::Test {
 public:
  SerpMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());

    // Register `kLastCheckYMD` pref (YYYY-MM-DD). This pref is part of the
    // daily usage ping and tracks the last reported day so we don't re-report
    // previously sent metrics.
    local_state_.registry()->RegisterStringPref(kLastCheckYMD,
                                                "");  // Never checked.

    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kSerpMetricsFeature, {{"time_period_in_days", "7"}});

    serp_metrics_ = std::make_unique<SerpMetrics>(&local_state_, &prefs_);
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

  base::test::ScopedFeatureList scoped_feature_list_;

  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple prefs_;

  std::unique_ptr<SerpMetrics> serp_metrics_;
};

TEST_F(SerpMetricsTest, NoSearchCountsWhenNoSearchesRecorded) {
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, NoBraveSearchCountForYesterday) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  AdvanceClockToNextDay();

  // Day 1: Yesterday (no searches)
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();

  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, BraveSearchCountForYesterday) {
  // Day 0: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();

  EXPECT_EQ(2U, serp_metrics_->GetBraveSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, NoGoogleSearchCountForYesterday) {
  // Day 0: Stale
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToNextDay();

  // Day 1: Yesterday (no searches)
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();

  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, GoogleSearchCountForYesterday) {
  // Day 0: Yesterday
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();

  EXPECT_EQ(2U, serp_metrics_->GetGoogleSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, NoOtherSearchCountForYesterday) {
  // Day 0: Stale
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 1: Yesterday (no searches)
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();

  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, OtherSearchCountForYesterday) {
  // Day 0: Yesterday
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();

  EXPECT_EQ(2U, serp_metrics_->GetOtherSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, SearchCountForYesterday) {
  // Day 0: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();

  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(2U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(3U, serp_metrics_->GetOtherSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, SearchCountForYesterdayOnCuspOfDayRollover) {
  // Day 0: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();

  AdvanceClockToJustBeforeNextDay();
  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(2U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(3U, serp_metrics_->GetOtherSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, SearchCountForYesterdayWhenTodayHasNoRecordedSearches) {
  // Day 0: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 1: Today (no searches)

  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(2U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(3U, serp_metrics_->GetOtherSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, DailyUsagePingIncludesYesterdayCounts) {
  // Verifies that yesterday’s searches are included when the last daily usage
  // ping was sent on the previous day.

  // Day 0: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  SimulateSendingDailyUsagePingAt(base::Time::Now());
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();

  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetOtherSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, BraveSearchCountForStalePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, GoogleSearchCountForStalePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, OtherSearchCountForStalePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, SearchCountForStalePeriodAcrossMultipleDays) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToNextDay();

  // Day 1: Stale
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 2: Stale
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 3: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 4: Today
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();

  EXPECT_EQ(6U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, SearchCountsForYesterdayAndStalePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordGoogleSearch();

  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(2U, serp_metrics_->GetOtherSearchCountForYesterday());
  EXPECT_EQ(4U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, DoNotCountSearchesBeforeLastDailyUsagePingWasSent) {
  // Verifies that sending the daily usage ping updates the reporting cutoff
  // and prevents re-reporting older searches.

  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToNextDay();

  // Day 1: Stale
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  const base::Time first_daily_usage_ping_at = base::Time::Now();
  AdvanceClockToNextDay();

  // Day 2: Stale
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 3: Yesterday
  serp_metrics_->RecordBraveSearch();
  const base::Time second_daily_usage_ping_at = base::Time::Now();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 4: Today
  serp_metrics_->RecordBraveSearch();

  // First daily usage ping. Searches from Day 0 have already been reported.
  // Searches from Day 1, Day 2, and yesterday are counted.
  SimulateSendingDailyUsagePingAt(first_daily_usage_ping_at);
  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(2U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(3U, serp_metrics_->GetOtherSearchCountForYesterday());
  EXPECT_EQ(4U, serp_metrics_->GetSearchCountForStalePeriod());

  // Second daily usage ping. Searches from Day 1 and Day 2 have already been
  // reported. Only yesterday’s searches are counted.
  SimulateSendingDailyUsagePingAt(second_daily_usage_ping_at);
  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(2U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(3U, serp_metrics_->GetOtherSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());

  // Final daily usage ping. Yesterday’s searches are already reported. We are
  // all caught up. Nothing else to include.
  SimulateSendingDailyUsagePingAt(base::Time::Now());
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, CountAllSearchesIfDailyUsagePingWasNeverSent) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  AdvanceClockToNextDay();

  // Day 1: Stale
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 2: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 3: Today
  serp_metrics_->RecordBraveSearch();

  local_state_.SetString(kLastCheckYMD, "");  // Never checked.
  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetOtherSearchCountForYesterday());
  EXPECT_EQ(3U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, CountAllSearchesIfLastCheckedDateIsInvalid) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  AdvanceClockToNextDay();

  // Day 1: Stale
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 2: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 3: Today
  serp_metrics_->RecordBraveSearch();

  local_state_.SetString(kLastCheckYMD, "invalid");
  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetOtherSearchCountForYesterday());
  EXPECT_EQ(3U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, DoNotCountSearchesWhenLastCheckedDateIsInFuture) {
  // Day 0: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordOtherSearch();

  SimulateSendingDailyUsagePingAt(
      base::Time::Now() +
      base::Days(365));  // Time travel for testing purposes only.
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, DoNotCountSearchesOutsideGivenRetentionPeriod) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockByRetentionPeriod();

  // Day 7: Stale (day 0 falls outside the retention window)
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 8: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToNextDay();

  // Day 9: Today (no searches)

  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetOtherSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, ClearHistoryClearsAllSearchCounts) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  ASSERT_EQ(1U, serp_metrics_->GetBraveSearchCountForTesting());
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordGoogleSearch();
  ASSERT_EQ(1U, serp_metrics_->GetGoogleSearchCountForTesting());
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordOtherSearch();
  ASSERT_EQ(1U, serp_metrics_->GetOtherSearchCountForTesting());

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForTesting());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForTesting());
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForTesting());
}

TEST_F(SerpMetricsTest, ClearHistoryDoesNotRestoreClearedSearchCounts) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  ASSERT_EQ(1U, serp_metrics_->GetBraveSearchCountForTesting());
  AdvanceClockToNextDay();

  // Day 2: Yesterday
  serp_metrics_->RecordGoogleSearch();
  ASSERT_EQ(1U, serp_metrics_->GetGoogleSearchCountForTesting());
  AdvanceClockToNextDay();

  // Day 3: Today
  serp_metrics_->RecordOtherSearch();
  ASSERT_EQ(1U, serp_metrics_->GetOtherSearchCountForTesting());

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForTesting());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForTesting());
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForTesting());

  serp_metrics_->RecordOtherSearch();
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForTesting());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForTesting());
  EXPECT_EQ(1U, serp_metrics_->GetOtherSearchCountForTesting());
}

TEST_F(SerpMetricsTest, ClearHistoryWithNoSearchesRecorded) {
  // Day 0: Today (no searches)

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForTesting());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForTesting());
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForTesting());
}

TEST_F(SerpMetricsTest, ClearHistoryDoesNotAffectDailyUsagePing) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordBraveSearch();
  SimulateSendingDailyUsagePingAt(base::Time::Now());
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_->RecordBraveSearch();

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForTesting());
}

TEST_F(SerpMetricsTest, ClearHistoryIsIdempotent) {
  // Day 0: Today
  serp_metrics_->RecordOtherSearch();

  serp_metrics_->ClearHistory();
  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForTesting());
}

}  // namespace serp_metrics
