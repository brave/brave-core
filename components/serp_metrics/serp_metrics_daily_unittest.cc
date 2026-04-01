/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/time/time.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/test/serp_metrics_test_base.h"

namespace serp_metrics {

// Day numbering is relative to the start of the test: Day 0 is the first day,
// Day 1 is the next day, and so on. "Today" always refers to the most recent
// day. Daily usage pings are simulated at specific times to verify which
// metrics are included or excluded based on `kLastReportedAt`. Usage pings
// report metrics from yesterday as well as from the stale period.
class SerpMetricsDailyTest : public SerpMetricsTestBase {
 public:
  // Advances the clock to one millisecond shy of a brand new UTC day.
  void AdvanceClockToJustBeforeNextDay() {
    const base::Time now = base::Time::Now();
    const base::Time end_of_day =
        now.UTCMidnight() + base::Days(1) - base::Milliseconds(1);
    task_environment_.AdvanceClock(end_of_day - now);
  }

  // Advances the clock to the start of a brand new UTC day.
  void AdvanceClockToNextDay() {
    task_environment_.AdvanceClock(base::Days(1));
  }

  // Advances the clock to exactly the next UTC midnight.
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

  // Simulates sending a daily usage ping by recording `at` as the last reported
  // time. `GetSearchCountForStalePeriod` uses `kLastReportedAt` to
  // determine which searches are already reported.
  void SimulateSendingDailyUsagePingAt(base::Time at) {
    local_state_.SetTime(prefs::kLastReportedAt, at);
  }
};

TEST_F(SerpMetricsDailyTest, NoSearchCountsWhenNoSearchesRecorded) {
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsDailyTest, NoBraveSearchCountForYesterday) {
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

TEST_F(SerpMetricsDailyTest, BraveSearchCountForYesterday) {
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

TEST_F(SerpMetricsDailyTest, NoGoogleSearchCountForYesterday) {
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

TEST_F(SerpMetricsDailyTest, GoogleSearchCountForYesterday) {
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

TEST_F(SerpMetricsDailyTest, NoOtherSearchCountForYesterday) {
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

TEST_F(SerpMetricsDailyTest, OtherSearchCountForYesterday) {
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

TEST_F(SerpMetricsDailyTest, SearchCountForYesterday) {
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

TEST_F(SerpMetricsDailyTest, SearchCountForYesterdayOnCuspOfDayRollover) {
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

TEST_F(SerpMetricsDailyTest,
       SearchCountForYesterdayWhenTodayHasNoRecordedSearches) {
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

TEST_F(SerpMetricsDailyTest, DailyUsagePingIncludesYesterdayCounts) {
  // Verifies that yesterday's searches are included when the last daily usage
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

TEST_F(SerpMetricsDailyTest, BraveSearchCountForStalePeriod) {
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

TEST_F(SerpMetricsDailyTest, GoogleSearchCountForStalePeriod) {
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

TEST_F(SerpMetricsDailyTest, OtherSearchCountForStalePeriod) {
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

TEST_F(SerpMetricsDailyTest, SearchCountForStalePeriodAcrossMultipleDays) {
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

TEST_F(SerpMetricsDailyTest, SearchCountsForYesterdayAndStalePeriod) {
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

TEST_F(SerpMetricsDailyTest, SearchCountForStalePeriodOnCuspOfDayRollover) {
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

TEST_F(SerpMetricsDailyTest,
       DoNotCountSearchesBeforeLastDailyUsagePingWasSent) {
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
  // reported. Only yesterday's searches are counted.
  SimulateSendingDailyUsagePingAt(second_daily_usage_ping_at);
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());

  // Final daily usage ping. Yesterday's searches are already reported. We are
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

TEST_F(SerpMetricsDailyTest, CountAllSearchesIfDailyUsagePingWasNeverSent) {
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

  // `kLastReportedAt` defaults to null — simulating a never-sent daily
  // ping.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(3U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsDailyTest, DoNotCountSearchesWhenLastCheckedDateIsInFuture) {
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

TEST_F(SerpMetricsDailyTest, DoNotCountSearchesOutsideGivenRetentionPeriod) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByRetentionPeriod();

  // Day 62: Stale (day 0 falls outside the retention window)
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 63: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 64: Today (no searches)

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsDailyTest, ClearHistoryClearsAllSearchCounts) {
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

TEST_F(SerpMetricsDailyTest, ClearHistoryDoesNotRestoreClearedSearchCounts) {
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

TEST_F(SerpMetricsDailyTest, ClearHistoryWithNoSearchesRecorded) {
  // Day 0: Today (no searches)

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsDailyTest, ClearHistoryDoesNotAffectDailyUsagePing) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  SimulateSendingDailyUsagePingAt(base::Time::Now());
  AdvanceClockToNextDay();

  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  serp_metrics_->ClearHistory();
  EXPECT_FALSE(local_state_.GetTime(prefs::kLastReportedAt).is_null());
}

TEST_F(SerpMetricsDailyTest, ClearHistoryIsIdempotent) {
  // Day 0: Today
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  serp_metrics_->ClearHistory();
  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsDailyTest,
       SearchRecordedJustBeforeUTCMidnightCountsAsYesterday) {
  // Day 0: Yesterday
  AdvanceClockToJustBeforeNextUTCMidnight();
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1: Today
  task_environment_.AdvanceClock(base::Milliseconds(1));

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsDailyTest, SearchRecordedAtExactUTCMidnightCountsAsToday) {
  // Day 0: Today
  AdvanceClockToNextUTCMidnight();
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsDailyTest, SearchRecordedJustAfterUTCMidnightCountsAsToday) {
  // Day 0: Today
  AdvanceClockToNextUTCMidnight();
  task_environment_.AdvanceClock(base::Milliseconds(1));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsDailyTest,
       SearchRecordedAtMiddayCountsAsYesterdayAfterMidnight) {
  // Day 0: Yesterday
  task_environment_.AdvanceClock(base::Hours(12));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1: Today
  AdvanceClockToNextDay();

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsDailyTest, SearchAtUTCMidnightEndsUpInYesterdayNotStale) {
  // Day 0: Stale (no searches)
  SimulateSendingDailyUsagePingAt(base::Time::Now());
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 2: Today
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsDailyTest,
       SearchJustBeforeUTCMidnightEndsUpInStaleNotYesterday) {
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

TEST_F(SerpMetricsDailyTest, LastCheckedAtOnYesterdayMidnightYieldsZeroStale) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  SimulateSendingDailyUsagePingAt(base::Time::Now());
  AdvanceClockToNextDay();

  // Day 2: Today
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsDailyTest,
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
  SimulateSendingDailyUsagePingAt(just_before_yesterday_midnight);

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsDailyTest,
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

TEST_F(SerpMetricsDailyTest,
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
