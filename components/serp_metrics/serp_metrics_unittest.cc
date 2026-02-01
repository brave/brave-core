/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics.h"

#include <cstddef>
#include <memory>
#include <string>

#include "absl/strings/str_format.h"
#include "base/check_op.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/pref_registry.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// Day numbering is relative to test start. Day 0 is the first day of the test;
// Day 1 is the next day, and so on. "Today" always refers to the latest day.

namespace serp_metrics {

namespace {

std::string NowAsYMD() {
  base::Time::Exploded now_exploded;
  base::Time::Now().LocalExplode(&now_exploded);
  return absl::StrFormat("%04d-%02d-%02d", now_exploded.year,
                         now_exploded.month, now_exploded.day_of_month);
}

}  // namespace

class SerpMetricsTest : public testing::Test {
 public:
  SerpMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());

    // Register last check date pref (YYYY-MM-DD). This pref is part of the
    // daily usage ping, and is used to determine the last day that was already
    // reported so we don't re-report metrics that have already been sent.
    local_state_.registry()->RegisterStringPref(kLastCheckYMD,
                                                "");  // Never checked.

    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kSerpMetricsFeature, {{"time_period_in_days", "7"}});

    serp_metrics_ = std::make_unique<SerpMetrics>(&local_state_, &prefs_);
  }

  // Advances the clock to local midnight at the beginning of the day that is
  // `days` calendar days from the current time. `days` must be greater than 0
  // to ensure the clock advances forward.
  void AdvanceClockToStartOfLocalDayAfterDays(size_t days) {
    CHECK_GT(days, 0U);

    base::Time now = base::Time::Now();
    base::Time distant_future_at_midnight =
        now.LocalMidnight() + base::Days(days);
    task_environment_.AdvanceClock(distant_future_at_midnight - now);
  }

 protected:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedFeatureList scoped_feature_list_;

  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple prefs_;

  std::unique_ptr<SerpMetrics> serp_metrics_;
};

TEST_F(SerpMetricsTest, NoBraveSearchCountForYesterday) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Yesterday (no recorded searches)
  AdvanceClockToStartOfLocalDayAfterDays(1);

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
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Today
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();

  EXPECT_EQ(2U, serp_metrics_->GetBraveSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, BraveSearchCountForStalePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 2: Today
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordBraveSearch();

  EXPECT_EQ(2U, serp_metrics_->GetBraveSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, NoGoogleSearchCountForYesterday) {
  // Day 0: Stale
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Yesterday (no recorded searches)
  AdvanceClockToStartOfLocalDayAfterDays(1);

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
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Today
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();

  EXPECT_EQ(2U, serp_metrics_->GetGoogleSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, GoogleSearchCountForStalePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Yesterday
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 2: Today
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();

  EXPECT_EQ(2U, serp_metrics_->GetGoogleSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, NoOtherSearchCountForYesterday) {
  // Day 0: Stale
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Yesterday (no recorded searches)
  AdvanceClockToStartOfLocalDayAfterDays(1);

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
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Today
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  EXPECT_EQ(2U, serp_metrics_->GetOtherSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, OtherSearchCountForStalePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Yesterday
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 2: Today
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();

  EXPECT_EQ(2U, serp_metrics_->GetOtherSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, DoNotCountSearchesOnOrBeforeLastCheckedDate) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  const std::string last_checked_at_1 = NowAsYMD();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Stale
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 2: Yesterday
  serp_metrics_->RecordBraveSearch();
  const std::string last_checked_at_2 = NowAsYMD();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 3: Today
  serp_metrics_->RecordBraveSearch();

  local_state_.SetString(kLastCheckYMD, last_checked_at_1);
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForStalePeriod());
  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetGoogleSearchCountForStalePeriod());
  EXPECT_EQ(1U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForStalePeriod());
  EXPECT_EQ(1U, serp_metrics_->GetOtherSearchCountForYesterday());

  local_state_.SetString(kLastCheckYMD, last_checked_at_2);
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForStalePeriod());
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForStalePeriod());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForStalePeriod());
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, SearchCountForStalePeriodOverMultipleDays) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Stale
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 2: Stale
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 3: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 4: Today
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();

  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForStalePeriod());
  EXPECT_EQ(2U, serp_metrics_->GetGoogleSearchCountForStalePeriod());
  EXPECT_EQ(3U, serp_metrics_->GetOtherSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, SearchCountForYesterday) {
  // Day 0: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

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
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Today
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  task_environment_.AdvanceClock(
      base::Days(1) -
      base::Milliseconds(1));  // One millisecond shy of a brand new day.

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
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Today (no recorded searches)

  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(2U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(3U, serp_metrics_->GetOtherSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, ZeroSearchCounts) {
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForStalePeriod());
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForStalePeriod());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForStalePeriod());
  EXPECT_EQ(0U, serp_metrics_->GetOtherSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, SearchCounts) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 1: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordOtherSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 2: Today
  serp_metrics_->RecordGoogleSearch();

  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForStalePeriod());
  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(2U, serp_metrics_->GetGoogleSearchCountForStalePeriod());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetOtherSearchCountForStalePeriod());
  EXPECT_EQ(2U, serp_metrics_->GetOtherSearchCountForYesterday());
}

TEST_F(SerpMetricsTest, ExpireSearchCountsAfterGivenTimePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();

  // Advance by the full retention period (`kSerpMetricsTimePeriodInDays` days).
  AdvanceClockToStartOfLocalDayAfterDays(kSerpMetricsTimePeriodInDays.Get());

  // Day 7: Stale
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 8: Yesterday
  serp_metrics_->RecordBraveSearch();
  serp_metrics_->RecordGoogleSearch();
  serp_metrics_->RecordOtherSearch();
  AdvanceClockToStartOfLocalDayAfterDays(1);

  // Day 9: Today
  EXPECT_EQ(0U, serp_metrics_->GetBraveSearchCountForStalePeriod());
  EXPECT_EQ(1U, serp_metrics_->GetBraveSearchCountForYesterday());
  EXPECT_EQ(0U, serp_metrics_->GetGoogleSearchCountForStalePeriod());
  EXPECT_EQ(1U, serp_metrics_->GetGoogleSearchCountForYesterday());
  EXPECT_EQ(1U, serp_metrics_->GetOtherSearchCountForStalePeriod());
  EXPECT_EQ(1U, serp_metrics_->GetOtherSearchCountForYesterday());
}

}  // namespace serp_metrics
