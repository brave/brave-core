/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/time/time.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/test/serp_metrics_calendar_test_util.h"
#include "brave/components/serp_metrics/test/serp_metrics_test_base.h"

namespace serp_metrics {

// Month numbering is relative to the start of the test: Month 0 is the first
// month, Month 1 is the next month, and so on. "This month" always refers to
// the most recent month. The clock is advanced to the 1st of the next month.
// The retention window covers two full calendar months,
// ensuring stale-month metrics remain within the `TimePeriodStorage` window.
// Monthly usage pings are simulated at specific times to verify which metrics
// are included or excluded based on `kLastMonthlyReportedAt`. Usage pings
// report metrics from last month as well as from the stale period.
class SerpMetricsMonthlyTest : public SerpMetricsTestBase {
 public:
  // Advances the clock to midnight UTC on the 1st of the next calendar month.
  void AdvanceClockByOneMonth() {
    const base::Time now = base::Time::Now();
    base::Time::Exploded exploded;
    now.UTCExplode(&exploded);
    const int days_in_month = DaysInMonth(exploded.year, exploded.month);
    const base::Time start_of_this_month =
        (now - base::Days(exploded.day_of_month - 1)).UTCMidnight();
    task_environment_.AdvanceClock(start_of_this_month +
                                   base::Days(days_in_month) - now);
  }

  // Advances the clock to one millisecond shy of the 1st of the next month at
  // midnight UTC.
  void AdvanceClockToJustBeforeNextMonth() {
    const base::Time now = base::Time::Now();
    base::Time::Exploded exploded;
    now.UTCExplode(&exploded);
    const int days_in_month = DaysInMonth(exploded.year, exploded.month);
    const base::Time start_of_this_month =
        (now - base::Days(exploded.day_of_month - 1)).UTCMidnight();
    const base::Time end_of_this_month =
        start_of_this_month + base::Days(days_in_month) - base::Milliseconds(1);
    task_environment_.AdvanceClock(end_of_this_month - now);
  }

  // Simulates sending a monthly usage ping by recording `at` as the last
  // monthly reported time. `GetSearchCountForStaleMonthPeriod` uses
  // `kLastMonthlyReportedAt` to determine which searches are already reported.
  void SimulateSendingMonthlyUsagePingAt(base::Time at) {
    local_state_.SetTime(prefs::kLastMonthlyReportedAt, at);
  }
};

TEST_F(SerpMetricsMonthlyTest, NoSearchCountsWhenNoSearchesRecorded) {
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest, NoBraveSearchCountForLastMonth) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneMonth();

  // Month 1: Last month (no searches)
  AdvanceClockByOneMonth();

  // Month 2: This month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsMonthlyTest, BraveSearchCountForLastMonth) {
  // Month 0: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneMonth();

  // Month 1: This month (ignored)
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsMonthlyTest, NoGoogleSearchCountForLastMonth) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneMonth();

  // Month 1: Last month (no searches)
  AdvanceClockByOneMonth();

  // Month 2: This month
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsMonthlyTest, GoogleSearchCountForLastMonth) {
  // Month 0: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneMonth();

  // Month 1: This month (ignored)
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
}

TEST_F(SerpMetricsMonthlyTest, NoOtherSearchCountForLastMonth) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 1: Last month (no searches)
  AdvanceClockByOneMonth();

  // Month 2: This month
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
}

TEST_F(SerpMetricsMonthlyTest, OtherSearchCountForLastMonth) {
  // Month 0: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 1: This month (ignored)
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
}

TEST_F(SerpMetricsMonthlyTest, SearchCountForLastMonth) {
  // Month 0: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 1: This month (ignored)
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
}

TEST_F(SerpMetricsMonthlyTest, SearchCountForLastMonthOnCuspOfMonthRollover) {
  // Month 0: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 1: This month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  AdvanceClockToJustBeforeNextMonth();
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
}

TEST_F(SerpMetricsMonthlyTest,
       SearchCountForLastMonthWhenThisMonthHasNoRecordedSearches) {
  // Month 0: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 1: This month (no searches)

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
}

TEST_F(SerpMetricsMonthlyTest, MonthlyUsagePingIncludesLastMonthCounts) {
  // Verifies that last month's searches are included when the last monthly
  // usage ping was sent during the previous month.

  // Month 0: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  SimulateSendingMonthlyUsagePingAt(base::Time::Now());
  AdvanceClockByOneMonth();

  // Month 1: This month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
}

TEST_F(SerpMetricsMonthlyTest, BraveSearchCountForStaleMonthPeriod) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneMonth();

  // Month 1: Last month (no searches)
  AdvanceClockByOneMonth();

  // Month 2: This month (no searches)

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest, GoogleSearchCountForStaleMonthPeriod) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneMonth();

  // Month 1: Last month (no searches)
  AdvanceClockByOneMonth();

  // Month 2: This month (no searches)

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest, OtherSearchCountForStaleMonthPeriod) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 1: Last month (no searches)
  AdvanceClockByOneMonth();

  // Month 2: This month (no searches)

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest, SearchCountsForLastMonthAndStaleMonthPeriod) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 1: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 2: This month (no searches)

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
  EXPECT_EQ(4U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest,
       SearchCountForStaleMonthPeriodOnCuspOfMonthRollover) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 1: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneMonth();

  // Month 2: This month (no searches)

  AdvanceClockToJustBeforeNextMonth();
  EXPECT_EQ(3U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest,
       DoNotCountSearchesBeforeLastMonthlyUsagePingWasSent) {
  // Verifies that sending the monthly usage ping updates the reporting cutoff
  // and prevents re-reporting older searches.

  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneMonth();

  // Month 1: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  const base::Time first_ping = base::Time::Now();
  AdvanceClockByOneMonth();

  // Month 2: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  const base::Time second_ping = base::Time::Now();
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 3: This month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // First monthly usage ping. Searches from Month 0 have already been
  // reported. Searches from Month 1 and last month are counted.
  SimulateSendingMonthlyUsagePingAt(first_ping);
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStaleMonthPeriod());

  // Second monthly usage ping. Searches from Month 1 have already been
  // reported. Only last month's searches are counted.
  SimulateSendingMonthlyUsagePingAt(second_ping);
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStaleMonthPeriod());

  // Final monthly usage ping. The stale range is now empty and last-month
  // counts are unaffected.
  SimulateSendingMonthlyUsagePingAt(base::Time::Now());
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest, CountAllSearchesIfMonthlyUsagePingWasNeverSent) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneMonth();

  // Month 1: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 2: This month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // `kLastMonthlyReportedAt` defaults to null — simulating a never-sent monthly
  // ping.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest,
       DoNotCountSearchesWhenLastCheckedDateIsInFuture) {
  // Month 0: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockByOneMonth();

  // Month 1: This month
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  SimulateSendingMonthlyUsagePingAt(
      base::Time::Now() +
      base::Days(365));  // Time travel for testing purposes only.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest, DoNotCountSearchesOutsideGivenRetentionPeriod) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByRetentionPeriod();

  // Month 2: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 3: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 4: This month (no searches)

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest, ClearHistoryClearsAllSearchCounts) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  AdvanceClockByOneMonth();

  // Month 1: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  AdvanceClockByOneMonth();

  // Month 2: This month
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

TEST_F(SerpMetricsMonthlyTest, ClearHistoryDoesNotRestoreClearedSearchCounts) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  AdvanceClockByOneMonth();

  // Month 1: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  ASSERT_EQ(1U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  AdvanceClockByOneMonth();

  // Month 2: This month
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

TEST_F(SerpMetricsMonthlyTest, ClearHistoryWithNoSearchesRecorded) {
  // Month 0: This month (no searches)

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsMonthlyTest, ClearHistoryDoesNotAffectMonthlyUsagePing) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneMonth();

  // Month 1: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  SimulateSendingMonthlyUsagePingAt(base::Time::Now());
  AdvanceClockByOneMonth();

  // Month 2: This month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsMonthlyTest, ClearHistoryIsIdempotent) {
  // Month 0: This month
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  serp_metrics_->ClearHistory();
  serp_metrics_->ClearHistory();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForTesting(SerpMetricType::kOther));
}

TEST_F(SerpMetricsMonthlyTest,
       NoSearchCountForLastMonthWhenSearchesAreFromThisMonth) {
  // Month 0: This month (no searches from last month)
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
}

TEST_F(SerpMetricsMonthlyTest,
       NoSearchCountForLastMonthWhenSearchesAreOlderThanLastMonth) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockByOneMonth();

  // Month 1: Last month (no searches)
  AdvanceClockByOneMonth();

  // Month 2: This month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
}

TEST_F(SerpMetricsMonthlyTest,
       SearchRecordedJustBeforeMonthRolloverCountsAsLastMonth) {
  AdvanceClockToJustBeforeNextMonth();  // Last ms of the current month UTC
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  task_environment_.AdvanceClock(base::Milliseconds(1));  // 1st 00:00:00 UTC
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsMonthlyTest,
       SearchRecordedAtExactMonthRolloverCountsAsThisMonth) {
  AdvanceClockToJustBeforeNextMonth();
  task_environment_.AdvanceClock(base::Milliseconds(1));  // 1st 00:00:00 UTC
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsMonthlyTest, SearchAtMonthRolloverEndsUpInLastMonthNotStale) {
  SimulateSendingMonthlyUsagePingAt(base::Time::Now());
  AdvanceClockToJustBeforeNextMonth();
  task_environment_.AdvanceClock(base::Milliseconds(1));  // 1st 00:00:00 UTC
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneMonth();
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest,
       SearchJustBeforeMonthRolloverEndsUpInStaleNotLastMonth) {
  AdvanceClockToJustBeforeNextMonth();
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);  // Last ms of month
  task_environment_.AdvanceClock(base::Milliseconds(1));
  AdvanceClockByOneMonth();
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest,
       LastCheckedAtOnLastMonthStartYieldsZeroStaleMonthPeriod) {
  // Month 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneMonth();

  // Month 1: Last month
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  SimulateSendingMonthlyUsagePingAt(base::Time::Now());
  AdvanceClockByOneMonth();

  // Month 2: This month — stale range is empty because ping covered last month.
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest,
       SearchRecordedOnDecember31UTCCountsAsLastMonthInJanuary) {
  base::Time time;
  ASSERT_TRUE(base::Time::FromUTCString("31 Dec 2049 12:34:56", &time));
  task_environment_.AdvanceClock(time - base::Time::Now());
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByOneMonth();
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

}  // namespace serp_metrics
