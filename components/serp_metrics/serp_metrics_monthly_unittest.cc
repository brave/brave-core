/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "absl/strings/str_format.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/test/serp_metrics_calendar_test_util.h"
#include "brave/components/serp_metrics/test/serp_metrics_test_base.h"

namespace serp_metrics {

// Month numbering is relative to the start of the test: Month 0 is the first
// month, Month 1 is the next month, and so on. "This month" always refers to
// the most recent month. The clock is advanced to the 1st of the next month to
// mirror the boundary at which `brave_stats_updater` detects a new month via
// `kLastCheckMonth`. The retention window covers two full calendar months,
// ensuring stale-month metrics remain within the `TimePeriodStorage` window.
// Monthly usage pings are simulated at specific times to verify which metrics
// are included or excluded based on `kLastCheckYMD`. Usage pings report metrics
// from last month as well as from the stale period.
class SerpMetricsMonthlyTest : public SerpMetricsTestBase {
 public:
  void AdvanceClockByOneMonth() {
    const base::Time now = base::Time::Now();
    base::Time::Exploded exploded;
    now.LocalExplode(&exploded);
    const int days_in_month = DaysInMonth(exploded.year, exploded.month);
    const base::Time start_of_this_month =
        (now - base::Days(exploded.day_of_month - 1)).LocalMidnight();
    task_environment_.AdvanceClock(start_of_this_month +
                                   base::Days(days_in_month) - now);
  }

  void AdvanceClockToJustBeforeNextMonth() {
    const base::Time now = base::Time::Now();
    base::Time::Exploded exploded;
    now.LocalExplode(&exploded);
    const int days_in_month = DaysInMonth(exploded.year, exploded.month);
    const base::Time start_of_this_month =
        (now - base::Days(exploded.day_of_month - 1)).LocalMidnight();
    const base::Time end_of_this_month =
        start_of_this_month + base::Days(days_in_month) - base::Milliseconds(1);
    task_environment_.AdvanceClock(end_of_this_month - now);
  }

  void SimulateSendingMonthlyUsagePingAt(base::Time at) {
    base::Time::Exploded exploded;
    at.LocalExplode(&exploded);
    local_state_.SetString(
        kLastCheckYMD, absl::StrFormat("%04d-%02d-%02d", exploded.year,
                                       exploded.month, exploded.day_of_month));
    local_state_.SetInteger(kLastCheckMonth, 1);
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

  local_state_.SetInteger(kLastCheckMonth, 0);  // Never checked.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForLastMonth(SerpMetricType::kOther));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStaleMonthPeriod());
}

TEST_F(SerpMetricsMonthlyTest, CountAllSearchesIfLastCheckedDateIsInvalid) {
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

  local_state_.SetInteger(kLastCheckMonth, 1);
  local_state_.SetString(kLastCheckYMD, "invalid");
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

}  // namespace serp_metrics
