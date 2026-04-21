/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "absl/strings/str_format.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/test/serp_metrics_test_base.h"

namespace serp_metrics {

// Week numbering is relative to the start of the test: Week 0 is the first
// week, Week 1 is the next week, and so on. "This week" always refers to the
// most recent week. The retention window covers enough weeks so that last-week
// searches are within the `TimePeriodStorage` window. Advancing the clock by
// exactly 7 days guarantees that any searches recorded before the advance fall
// within the previous ISO week, regardless of the day of the week the test
// clock starts on. Weekly usage pings are simulated at specific times to verify
// which metrics are included or excluded based on `kLastCheckYMD`. Usage pings
// report metrics from last week as well as from the stale period.
class SerpMetricsWeeklyTest : public SerpMetricsTestBase {
 public:
  void AdvanceClockByOneWeek() {
    task_environment_.AdvanceClock(base::Days(7));
  }

  void AdvanceClockToJustBeforeNextWeek() {
    const base::Time now = base::Time::Now();
    base::Time::Exploded exploded;
    now.LocalExplode(&exploded);
    const int days_since_monday =
        (exploded.day_of_week == 0) ? 6 : exploded.day_of_week - 1;
    const base::Time start_of_this_week =
        (now - base::Days(days_since_monday)).LocalMidnight();
    const base::Time end_of_this_week =
        start_of_this_week + base::Days(7) - base::Milliseconds(1);
    task_environment_.AdvanceClock(end_of_this_week - now);
  }

  void SimulateSendingWeeklyUsagePingAt(base::Time at) {
    base::Time::Exploded exploded;
    at.LocalExplode(&exploded);
    local_state_.SetString(
        kLastCheckYMD, absl::StrFormat("%04d-%02d-%02d", exploded.year,
                                       exploded.month, exploded.day_of_month));
    local_state_.SetInteger(kLastCheckWOY, 1);
  }
};

TEST_F(SerpMetricsWeeklyTest, NoSearchCountsWhenNoSearchesRecorded) {
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStaleWeekPeriod());
}

TEST_F(SerpMetricsWeeklyTest, NoBraveSearchCountForLastWeek) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneWeek();

  // Week 1: Last week (no searches)
  AdvanceClockByOneWeek();

  // Week 2: This week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsWeeklyTest, BraveSearchCountForLastWeek) {
  // Week 0: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneWeek();

  // Week 1: This week (ignored)
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsWeeklyTest, NoGoogleSearchCountForLastWeek) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneWeek();

  // Week 1: Last week (no searches)
  AdvanceClockByOneWeek();

  // Week 2: This week
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsWeeklyTest, GoogleSearchCountForLastWeek) {
  // Week 0: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneWeek();

  // Week 1: This week (ignored)
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsWeeklyTest, NoOtherSearchCountForLastWeek) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 1: Last week (no searches)
  AdvanceClockByOneWeek();

  // Week 2: This week
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
}

TEST_F(SerpMetricsWeeklyTest, OtherSearchCountForLastWeek) {
  // Week 0: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 1: This week (ignored)
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
}

TEST_F(SerpMetricsWeeklyTest, SearchCountForLastWeek) {
  // Week 0: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 1: This week (ignored)
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
}

TEST_F(SerpMetricsWeeklyTest, SearchCountForLastWeekOnCuspOfWeekRollover) {
  // Week 0: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 1: This week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  AdvanceClockToJustBeforeNextWeek();
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
}

TEST_F(SerpMetricsWeeklyTest,
       SearchCountForLastWeekWhenThisWeekHasNoRecordedSearches) {
  // Week 0: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 1: This week (no searches)

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
}

TEST_F(SerpMetricsWeeklyTest, WeeklyUsagePingIncludesLastWeekCounts) {
  // Verifies that last week's searches are included when the last weekly usage
  // ping was sent during the previous week.

  // Week 0: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  SimulateSendingWeeklyUsagePingAt(base::Time::Now());
  AdvanceClockByOneWeek();

  // Week 1: This week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
}

TEST_F(SerpMetricsWeeklyTest, BraveSearchCountForStaleWeekPeriod) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneWeek();

  // Week 1: Last week (no searches)
  AdvanceClockByOneWeek();

  // Week 2: This week (no searches)

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStaleWeekPeriod());
}

TEST_F(SerpMetricsWeeklyTest, GoogleSearchCountForStaleWeekPeriod) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneWeek();

  // Week 1: Last week (no searches)
  AdvanceClockByOneWeek();

  // Week 2: This week (no searches)

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStaleWeekPeriod());
}

TEST_F(SerpMetricsWeeklyTest, OtherSearchCountForStaleWeekPeriod) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 1: Last week (no searches)
  AdvanceClockByOneWeek();

  // Week 2: This week (no searches)

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStaleWeekPeriod());
}

TEST_F(SerpMetricsWeeklyTest, SearchCountsForLastWeekAndStaleWeekPeriod) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 1: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 2: This week (no searches)

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
  EXPECT_EQ(4U, serp_metrics_->GetSearchCountForStaleWeekPeriod());
}

TEST_F(SerpMetricsWeeklyTest,
       SearchCountForStaleWeekPeriodOnCuspOfWeekRollover) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 1: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneWeek();

  // Week 2: This week (no searches)

  AdvanceClockToJustBeforeNextWeek();
  EXPECT_EQ(3U, serp_metrics_->GetSearchCountForStaleWeekPeriod());
}

TEST_F(SerpMetricsWeeklyTest,
       DoNotCountSearchesBeforeLastWeeklyUsagePingWasSent) {
  // Verifies that sending the weekly usage ping updates the reporting cutoff
  // and prevents re-reporting older searches.

  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneWeek();

  // Week 1: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  const base::Time first_weekly_usage_ping_at = base::Time::Now();
  AdvanceClockByOneWeek();

  // Week 2: Stale (no searches)
  AdvanceClockByOneWeek();

  // Week 3: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  const base::Time second_weekly_usage_ping_at = base::Time::Now();
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 4: This week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // First weekly usage ping. Searches from Week 0 have already been reported.
  // Searches from Week 1 and last week are counted.
  SimulateSendingWeeklyUsagePingAt(first_weekly_usage_ping_at);
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStaleWeekPeriod());

  // Second weekly usage ping. Searches from Week 0 and Week 1 have already
  // been reported. Only last week's searches are counted.
  SimulateSendingWeeklyUsagePingAt(second_weekly_usage_ping_at);
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStaleWeekPeriod());

  // Final weekly usage ping. The stale range is now empty and last-week
  // counts are unaffected.
  SimulateSendingWeeklyUsagePingAt(base::Time::Now());
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStaleWeekPeriod());
}

TEST_F(SerpMetricsWeeklyTest, CountAllSearchesIfWeeklyUsagePingWasNeverSent) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneWeek();

  // Week 1: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 2: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 3: This week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  local_state_.SetInteger(kLastCheckWOY, 0);  // Never checked.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
  EXPECT_EQ(3U, serp_metrics_->GetSearchCountForStaleWeekPeriod());
}

TEST_F(SerpMetricsWeeklyTest, CountAllSearchesIfLastCheckedDateIsInvalid) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneWeek();

  // Week 1: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 2: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 3: This week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  local_state_.SetInteger(kLastCheckWOY, 1);
  local_state_.SetString(kLastCheckYMD, "invalid");
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
  EXPECT_EQ(3U, serp_metrics_->GetSearchCountForStaleWeekPeriod());
}

TEST_F(SerpMetricsWeeklyTest, DoNotCountSearchesWhenLastCheckedDateIsInFuture) {
  // Week 0: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneWeek();

  // Week 1: This week
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  SimulateSendingWeeklyUsagePingAt(
      base::Time::Now() +
      base::Days(365));  // Time travel for testing purposes only.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStaleWeekPeriod());
}

TEST_F(SerpMetricsWeeklyTest, DoNotCountSearchesOutsideGivenRetentionPeriod) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByRetentionPeriod();

  // Week 9: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 10: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 11: This week (no searches)

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStaleWeekPeriod());
}

TEST_F(SerpMetricsWeeklyTest, ClearHistoryClearsAllSearchCounts) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  AdvanceClockByOneWeek();

  // Week 1: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  AdvanceClockByOneWeek();

  // Week 2: This week
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

TEST_F(SerpMetricsWeeklyTest, ClearHistoryDoesNotRestoreClearedSearchCounts) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  AdvanceClockByOneWeek();

  // Week 1: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  AdvanceClockByOneWeek();

  // Week 2: This week
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

TEST_F(SerpMetricsWeeklyTest, ClearHistoryWithNoSearchesRecorded) {
  // Week 0: This week (no searches)

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsWeeklyTest, ClearHistoryDoesNotAffectWeeklyUsagePing) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneWeek();

  // Week 1: Last week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  SimulateSendingWeeklyUsagePingAt(base::Time::Now());
  AdvanceClockByOneWeek();

  // Week 2: This week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsWeeklyTest, ClearHistoryIsIdempotent) {
  // Week 0: This week
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  serp_metrics_->ClearHistory();
  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsWeeklyTest,
       NoSearchCountForLastWeekWhenSearchesAreFromThisWeek) {
  // Week 0: This week (no searches from last week)
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
}

TEST_F(SerpMetricsWeeklyTest,
       NoSearchCountForLastWeekWhenSearchesAreOlderThanLastWeek) {
  // Week 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneWeek();

  // Week 1: Last week (no searches)
  AdvanceClockByOneWeek();

  // Week 2: This week
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastWeek(SerpMetricType::kOther));
}

}  // namespace serp_metrics
