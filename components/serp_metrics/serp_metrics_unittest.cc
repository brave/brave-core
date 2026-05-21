/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics.h"

#include <memory>
#include <string>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/serp_metrics/test/fake_serp_metrics_time_period_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// Day numbering is relative to the start of the test: Day 0 is the first day,
// Day 1 is the next day, and so on. "Today" always refers to the most recent
// day. Daily usage pings are simulated at specific times to verify which
// metrics are included or excluded based on `kLastReportedAt`. Usage pings
// report metrics from yesterday as well as from the stale period.

namespace serp_metrics {

class SerpMetricsTest : public testing::Test {
 public:
  SerpMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    // Register `kLastCheckYMD` pref (YYYY-MM-DD). This pref is part of the
    // daily usage ping and tracks the last reported day so we don't re-report
    // previously sent metrics.
    local_state_.registry()->RegisterStringPref(kLastCheckYMD,
                                                "");  // Never checked.
    local_state_.registry()->RegisterTimePref(
        prefs::kLastReportedAt,
        /* Never reported */ base::Time());

    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kSerpMetricsFeature, {{"time_period_in_days", "7"}});

    serp_metrics_ = std::make_unique<SerpMetrics>(
        &local_state_, test::FakeSerpMetricsTimePeriodStoreFactory());
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

  void AdvanceClockToNextUTCMidnight() {
    const base::Time now = base::Time::Now();
    task_environment_.AdvanceClock(now.UTCMidnight() + base::Days(1) - now);
  }

  // Advances the clock to one millisecond shy of the next UTC midnight.
  void AdvanceClockToJustBeforeNextUTCMidnight() {
    const base::Time now = base::Time::Now();
    task_environment_.AdvanceClock(now.UTCMidnight() + base::Days(1) -
                                   base::Milliseconds(1) - now);
  }

 protected:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedFeatureList scoped_feature_list_;

  TestingPrefServiceSimple local_state_;

  std::unique_ptr<SerpMetrics> serp_metrics_;
};

TEST_F(SerpMetricsTest, NoSearchCountsWhenNoSearchesRecorded) {
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, NoBraveSearchCountForYesterday) {
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

TEST_F(SerpMetricsTest, BraveSearchCountForYesterday) {
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

TEST_F(SerpMetricsTest, NoGoogleSearchCountForYesterday) {
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

TEST_F(SerpMetricsTest, GoogleSearchCountForYesterday) {
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

TEST_F(SerpMetricsTest, NoOtherSearchCountForYesterday) {
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

TEST_F(SerpMetricsTest, OtherSearchCountForYesterday) {
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

TEST_F(SerpMetricsTest, SearchCountForYesterday) {
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

TEST_F(SerpMetricsTest, SearchCountForYesterdayOnCuspOfDayRollover) {
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

  AdvanceClockToJustBeforeNextUTCMidnight();
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTest, SearchCountForYesterdayWhenTodayHasNoRecordedSearches) {
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

TEST_F(SerpMetricsTest, DailyUsagePingIncludesYesterdayCounts) {
  // Verifies that yesterday's searches are included when the last daily usage
  // ping was sent on the previous day.

  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());
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

TEST_F(SerpMetricsTest, BraveSearchCountForStalePeriod) {
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

TEST_F(SerpMetricsTest, GoogleSearchCountForStalePeriod) {
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

TEST_F(SerpMetricsTest, OtherSearchCountForStalePeriod) {
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

TEST_F(SerpMetricsTest, StaleCountIsZeroWhenNoSearchesPrecedeYesterday) {
  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, SearchCountForStalePeriodAcrossMultipleDays) {
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

TEST_F(SerpMetricsTest, SearchCountsForYesterdayAndStalePeriod) {
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

TEST_F(SerpMetricsTest, SearchCountForStalePeriodOnCuspOfDayRollover) {
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

  AdvanceClockToJustBeforeNextUTCMidnight();
  EXPECT_EQ(3U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, DoNotCountSearchesBeforeLastDailyUsagePingWasSent) {
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
  local_state_.SetTime(prefs::kLastReportedAt, first_daily_usage_ping_at);
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(4U, serp_metrics_->GetSearchCountForStalePeriod());

  // Second daily usage ping. Searches from Day 1 and Day 2 have already been
  // reported. Only yesterday's searches are counted.
  local_state_.SetTime(prefs::kLastReportedAt, second_daily_usage_ping_at);
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());

  // Final daily usage ping. Yesterday's searches are already reported. We are
  // all caught up. Nothing else to include.
  local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, DailyPingTodaySuppressesBothYesterdayAndStaleCounts) {
  // Day 0: Stale (no searches)
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 2: Today
  local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, CountAllSearchesIfDailyUsagePingWasNeverSent) {
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

TEST_F(SerpMetricsTest, DontCountSearchesIfLegacyDateIsUnparseable) {
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
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, DoNotCountSearchesWhenLastCheckedDateIsInFuture) {
  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 1: Today
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  local_state_.SetTime(
      prefs::kLastReportedAt,
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

TEST_F(SerpMetricsTest, DoNotCountSearchesOutsideGivenRetentionPeriod) {
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

TEST_F(SerpMetricsTest, ClearHistoryClearsAllSearchCounts) {
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

TEST_F(SerpMetricsTest, ClearHistoryDoesNotRestoreClearedSearchCounts) {
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

TEST_F(SerpMetricsTest, ClearHistoryWithNoSearchesRecorded) {
  // Day 0: Today (no searches)

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTest, ClearHistoryDoesNotAffectDailyUsagePing) {
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  const base::Time ping_time = base::Time::Now();
  local_state_.SetTime(prefs::kLastReportedAt, ping_time);
  AdvanceClockToNextDay();

  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  serp_metrics_->ClearHistory();
  EXPECT_EQ(ping_time, local_state_.GetTime(prefs::kLastReportedAt));
}

TEST_F(SerpMetricsTest, ClearHistoryIsIdempotent) {
  // Day 0: Today
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  serp_metrics_->ClearHistory();
  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsTest, SearchRecordedJustBeforeUTCMidnightCountsAsYesterday) {
  // Day 0: Yesterday
  AdvanceClockToJustBeforeNextUTCMidnight();
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1: Today
  task_environment_.AdvanceClock(base::Milliseconds(1));

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTest, SearchRecordedAtExactUTCMidnightCountsAsToday) {
  // Day 0: Today
  AdvanceClockToNextUTCMidnight();
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTest, SearchRecordedJustAfterUTCMidnightCountsAsToday) {
  // Day 0: Today
  AdvanceClockToNextUTCMidnight();
  task_environment_.AdvanceClock(base::Milliseconds(1));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTest, SearchRecordedAtMiddayCountsAsYesterdayAfterMidnight) {
  // Day 0: Yesterday
  task_environment_.AdvanceClock(base::Hours(12));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1: Today
  AdvanceClockToNextDay();

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTest, SearchAtUTCMidnightEndsUpInYesterdayNotStale) {
  // Day 0: Stale (no searches)
  local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 2: Today
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, SearchJustBeforeUTCMidnightEndsUpInStaleNotYesterday) {
  // Day 0: Stale
  AdvanceClockToJustBeforeNextUTCMidnight();
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1: Yesterday
  task_environment_.AdvanceClock(base::Milliseconds(1));

  // Day 2: Today
  AdvanceClockToNextDay();

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, LastCheckedAtOnYesterdayMidnightYieldsZeroStale) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());
  AdvanceClockToNextDay();

  // Day 2: Today
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       LastCheckedAtJustBeforeYesterdayMidnightYieldsOneStaleDay) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 2: Today
  const base::Time just_before_yesterday_midnight =
      base::Time::Now().UTCMidnight() - base::Days(1) - base::Milliseconds(1);
  local_state_.SetTime(prefs::kLastReportedAt, just_before_yesterday_midnight);

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       SearchRecordedOnDecember31UTCCountsAsYesterdayOnJanuary1) {
  base::Time time;
  ASSERT_TRUE(base::Time::FromUTCString("31 Dec 2049 12:34:56", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());

  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Today
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       SearchRecordedOnLeapDayUTCIsReportedAsYesterdayOnMarch1) {
  base::Time time;
  ASSERT_TRUE(base::Time::FromUTCString("29 Feb 2052 12:00:00", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());

  // Day 0: Yesterday (leap day)
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Today
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

}  // namespace serp_metrics
