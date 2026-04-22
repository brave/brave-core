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
#include "third_party/abseil-cpp/absl/strings/str_format.h"

// Day numbering is relative to the start of the test: Day 0 is the first day,
// Day 1 is the next day, and so on. "Today" always refers to the most recent
// day. Daily usage pings are simulated at specific times to verify which
// metrics are included or excluded based on `kLastReportedAt`. Usage pings
// report metrics from yesterday as well as from the stale period.

namespace serp_metrics {

std::string FormatUTCDateAsYMD(base::Time time) {
  base::Time::Exploded exploded;
  time.UTCExplode(&exploded);
  return absl::StrFormat("%04d-%02d-%02d", exploded.year, exploded.month,
                         exploded.day_of_month);
}

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
    local_state_.registry()->RegisterTimePref(prefs::kLastReportedAt,
                                              base::Time());

    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kSerpMetricsFeature, {{"time_period_in_days", "7"}});

    serp_metrics_ = std::make_unique<SerpMetrics>(
        &local_state_, test::FakeSerpMetricsTimePeriodStoreFactory());
  }

  // Advances the clock to one millisecond shy of a brand new day.
  void AdvanceClockToJustBeforeNextDay() {
    const base::Time now = base::Time::Now();
    const base::Time end_of_day =
        now.UTCMidnight() + base::Days(1) - base::Milliseconds(1);
    task_environment_.AdvanceClock(end_of_day - now);
  }

  // Advances the clock to the next UTC midnight.
  void AdvanceClockToNextUTCMidnight() {
    const base::Time now = base::Time::Now();
    task_environment_.AdvanceClock(now.UTCMidnight() + base::Days(1) - now);
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

  // Simulates sending the daily usage ping by updating `kLastReportedAt`.
  void SimulateSendingDailyUsagePingAt(base::Time at) {
    local_state_.SetTime(prefs::kLastReportedAt, at);
  }

  // Sets the legacy `kLastCheckYMD` migration pref to the UTC date of `at`.
  void SetLastCheckYMD(base::Time at) {
    local_state_.SetString(kLastCheckYMD, FormatUTCDateAsYMD(at));
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

  AdvanceClockToJustBeforeNextDay();
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

  AdvanceClockToJustBeforeNextDay();
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

TEST_F(SerpMetricsTest, DailyPingTodaySuppressesBothYesterdayAndStaleCounts) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 2: Today. `kLastReportedAt.UTCMidnight()` equals today's midnight, so
  // the stale and yesterday windows both collapse to empty ranges.
  SimulateSendingDailyUsagePingAt(base::Time::Now());

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

TEST_F(SerpMetricsTest, CountAllSearchesIfLegacyDateIsUnparseable) {
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

TEST_F(SerpMetricsTest, DoNotCountSearchesWhenLastCheckedDateIsInFuture) {
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

TEST_F(
    SerpMetricsTest,
    StaleCountIsLimitedByRetentionWindowWhenLastReportedAtIsOlderThanRetention) {
  // Day 0: Will fall outside the 7-day retention window by the time we check.
  const base::Time day_0 = base::Time::Now();
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockByRetentionPeriod();

  // Day 7: Stale, inside the retention window.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 8: Yesterday.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 9: Today. `kLastReportedAt` = Day 0, which is older than the 7-day
  // retention window. The 2 Day 0 searches are dropped by the storage; only
  // the 1 Day 7 search falls within the stale range.
  SimulateSendingDailyUsagePingAt(day_0);
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
  SimulateSendingDailyUsagePingAt(ping_time);
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
  AdvanceClockToJustBeforeNextDay();

  // Record 1 ms before UTC midnight.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Cross into the next UTC day.
  task_environment_.AdvanceClock(base::Milliseconds(1));

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTest, SearchRecordedAtExactUTCMidnightCountsAsToday) {
  AdvanceClockToNextUTCMidnight();

  // Exactly at UTC midnight = start of today.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTest, SearchRecordedJustAfterUTCMidnightCountsAsToday) {
  AdvanceClockToNextUTCMidnight();
  task_environment_.AdvanceClock(base::Milliseconds(1));

  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTest, SearchRecordedAtMiddayCountsAsYesterdayAfterMidnight) {
  task_environment_.AdvanceClock(base::Hours(12));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  AdvanceClockToNextUTCMidnight();

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTest, SearchAtUTCMidnightEndsUpInYesterdayNotStale) {
  // Day 0: Baseline ping sent at end of day.
  SimulateSendingDailyUsagePingAt(base::Time::Now());
  AdvanceClockToNextUTCMidnight();

  // Day 1: Search at exactly UTC midnight = start of yesterday.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, SearchJustBeforeUTCMidnightEndsUpInStaleNotYesterday) {
  AdvanceClockToJustBeforeNextDay();

  // Record at Day 0 UTCMidnight − 1 ms.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1: Yesterday.
  task_environment_.AdvanceClock(base::Milliseconds(1));
  // Day 2: Today.
  AdvanceClockToNextUTCMidnight();

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
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today. `kLastReportedAt` = yesterday UTC midnight, so stale start
  // equals stale end plus 1 ms, which makes the stale count 0.
  const base::Time yesterday_midnight =
      base::Time::Now().UTCMidnight() - base::Days(1);
  SimulateSendingDailyUsagePingAt(yesterday_midnight);

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
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today. `kLastReportedAt` = 1 ms before yesterday midnight,
  // so the stale period includes all of Day 0.
  const base::Time just_before_yesterday_midnight =
      base::Time::Now().UTCMidnight() - base::Days(1) - base::Milliseconds(1);
  SimulateSendingDailyUsagePingAt(just_before_yesterday_midnight);

  EXPECT_EQ(2U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       SearchesBeforePingInSameUTCDayDeferredToNextPingInPositiveUTCOffset) {
  // Simulates UTC+8 (ping fires at UTC 16:00, partway through the UTC day).
  // Advance to a known UTC midnight so hour offsets are deterministic.
  AdvanceClockToNextUTCMidnight();  // Day 0 midnight UTC.

  // Day 0, UTC 12:00. Record 2 searches in the UTC day 0 bucket.
  task_environment_.AdvanceClock(base::Hours(12));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 0, UTC 16:00. Ping fires (local midnight in UTC+8).
  task_environment_.AdvanceClock(base::Hours(4));
  // Yesterday UTC is [Day -1 midnight, Day 0 midnight - 1ms]. No searches.
  // Searches at UTC 12:00 are in today's UTC day 0 bucket, not yesterday.
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
  SimulateSendingDailyUsagePingAt(base::Time::Now());

  // Day 1, UTC 16:00. Next ping fires (local midnight in UTC+8).
  task_environment_.AdvanceClock(base::Hours(24));
  // Yesterday UTC is [Day 0 midnight, Day 1 midnight - 1ms].
  // `GetStartOfStalePeriod` returns UTCMidnight(Day 0 16:00) = Day 0 midnight.
  // Searches from Day 0 12:00 are in the Day 0 UTC bucket, within yesterday.
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       SearchesBeforePingInSameUTCDayDeferredToNextPingInNegativeUTCOffset) {
  // Simulates UTC-8 (ping fires at UTC 08:00, partway through the UTC day).
  // Advance to a known UTC midnight so hour offsets are deterministic.
  AdvanceClockToNextUTCMidnight();  // Day 0 midnight UTC.

  // Day 0, UTC 02:00 through 06:00. Record 3 searches in the UTC day 0 bucket.
  task_environment_.AdvanceClock(base::Hours(2));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  task_environment_.AdvanceClock(base::Hours(2));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  task_environment_.AdvanceClock(base::Hours(2));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 0, UTC 08:00. Ping fires (local midnight in UTC-8).
  task_environment_.AdvanceClock(base::Hours(2));
  // Yesterday UTC is [Day -1 midnight, Day 0 midnight - 1ms]. No searches.
  // Searches at UTC 02:00 through 06:00 are in today's UTC day 0 bucket.
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
  SimulateSendingDailyUsagePingAt(base::Time::Now());

  // Day 1, UTC 08:00. Next ping fires (local midnight in UTC-8).
  task_environment_.AdvanceClock(base::Hours(24));
  // Yesterday UTC is [Day 0 midnight, Day 1 midnight - 1ms].
  // `GetStartOfStalePeriod` returns UTCMidnight(Day 0 08:00) = Day 0 midnight.
  // Searches from Day 0 are in the Day 0 UTC bucket, within yesterday.
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       SearchAfterPingInSameUTCDayReportedAtNextPingInNegativeUTCOffset) {
  // Simulates UTC-8 (ping fires at UTC 08:00). A search is made later that
  // same UTC day at UTC 20:00. Although that time is local noon — the user's
  // "today" — the UTC day has not yet rolled over, so the search is in UTC
  // day 0. At the next ping (Day 1 UTC 08:00) it is correctly reported as
  // yesterday.
  AdvanceClockToNextUTCMidnight();  // Day 0 midnight UTC.

  // Day 0, UTC 08:00. First ping fires (local midnight in UTC-8).
  task_environment_.AdvanceClock(base::Hours(8));
  SimulateSendingDailyUsagePingAt(base::Time::Now());

  // Day 0, UTC 20:00. Search at local 12:00 in UTC-8 (same local day as ping).
  task_environment_.AdvanceClock(base::Hours(12));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  // This search is in the UTC day 0 bucket.

  // Day 1, UTC 08:00. Next ping fires (local midnight in UTC-8).
  task_environment_.AdvanceClock(base::Hours(12));
  // Yesterday UTC is [Day 0 midnight, Day 1 midnight - 1ms].
  // `GetStartOfStalePeriod` returns UTCMidnight(Day 0 08:00) = Day 0 midnight.
  // The Day 0 20:00 search is in UTC day 0, within yesterday.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       SearchAfterPingInSameUTCDayReportedAtNextPingInPositiveUTCOffset) {
  // Simulates UTC+8 (ping fires at UTC 16:00). A search is made just after
  // the ping at UTC 16:30, still within UTC day 0. At the next ping
  // (Day 1 UTC 16:00), UTC day 0 is yesterday, so the search is correctly
  // reported as yesterday.
  AdvanceClockToNextUTCMidnight();  // Day 0 midnight UTC.

  // Day 0, UTC 16:00: first ping fires (local midnight in UTC+8).
  task_environment_.AdvanceClock(base::Hours(16));
  SimulateSendingDailyUsagePingAt(base::Time::Now());

  // Day 0, UTC 16:30: search just after the ping.
  task_environment_.AdvanceClock(base::Minutes(30));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  // This search is in the UTC day 0 bucket (16:30 UTC is still UTC day 0).

  // Day 1, UTC 16:00: second ping fires (local midnight in UTC+8).
  task_environment_.AdvanceClock(base::Hours(23) + base::Minutes(30));
  // now.UTCMidnight() = Day 1 midnight UTC.
  // Yesterday UTC = [Day 0 midnight, Day 1 midnight - 1ms].
  // `GetStartOfStalePeriod` = UTCMidnight(Day 0 16:00) = Day 0 midnight.
  // The Day 0 16:30 search is in the Day 0 UTC bucket, which is in yesterday.
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       AllSearchesInPingUTCDayReportedAtNextPingForWidestPositiveOffset) {
  // Simulates UTC+13 (Pacific/Apia), the widest positive offset. The ping
  // fires at UTC 11:00, 11 hours into the UTC day.
  AdvanceClockToNextUTCMidnight();  // Day 0 midnight UTC.

  // Day 0, UTC 06:00. Record a search before the ping.
  task_environment_.AdvanceClock(base::Hours(6));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 0, UTC 11:00. Ping fires (local midnight in UTC+13).
  task_environment_.AdvanceClock(base::Hours(5));
  // Yesterday UTC is [Day -1 midnight, Day 0 midnight - 1ms]. No searches.
  // The UTC 06:00 search is in today's UTC day 0 bucket, not yesterday.
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
  SimulateSendingDailyUsagePingAt(base::Time::Now());

  // Day 0, UTC 17:00. Record a search after the ping, still in UTC day 0.
  task_environment_.AdvanceClock(base::Hours(6));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1, UTC 11:00. Next ping fires (local midnight in UTC+13).
  task_environment_.AdvanceClock(base::Hours(18));
  // Yesterday UTC is [Day 0 midnight, Day 1 midnight - 1ms].
  // `GetStartOfStalePeriod` returns UTCMidnight(Day 0 11:00) = Day 0 midnight.
  // Both searches from Day 0 are in the Day 0 UTC bucket, within yesterday.
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       AllSearchesInPingUTCDayReportedAtNextPingForWidestNegativeOffset) {
  // Simulates UTC-11 (Pacific/Pago_Pago), the widest negative offset. The
  // ping fires at UTC 11:00, 11 hours into the UTC day. Though opposite in
  // sign to UTC+13, the ping fires at the same UTC hour; having both tests
  // guards against sign-flip regressions.
  AdvanceClockToNextUTCMidnight();  // Day 0 midnight UTC.

  // Day 0, UTC 02:00 and UTC 08:00. Record 2 searches before the ping.
  task_environment_.AdvanceClock(base::Hours(2));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  task_environment_.AdvanceClock(base::Hours(6));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 0, UTC 11:00. Ping fires (local midnight in UTC-11).
  task_environment_.AdvanceClock(base::Hours(3));
  // Yesterday UTC is [Day -1 midnight, Day 0 midnight - 1ms]. No searches.
  // Searches at UTC 02:00 and 08:00 are in today's UTC day 0 bucket.
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
  SimulateSendingDailyUsagePingAt(base::Time::Now());

  // Day 0, UTC 18:00. Record a search after the ping, still in UTC day 0.
  task_environment_.AdvanceClock(base::Hours(7));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1, UTC 11:00. Next ping fires (local midnight in UTC-11).
  task_environment_.AdvanceClock(base::Hours(17));
  // Yesterday UTC is [Day 0 midnight, Day 1 midnight - 1ms].
  // `GetStartOfStalePeriod` returns UTCMidnight(Day 0 11:00) = Day 0 midnight.
  // All 3 searches from Day 0 are in the Day 0 UTC bucket, within yesterday.
  EXPECT_EQ(3U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       AllSearchesInPingUTCDayReportedAtNextPingForFractionalUTCOffset) {
  // Simulates UTC+5:45 (Asia/Kathmandu). The ping fires at UTC 18:15; the
  // fractional offset places it mid-way through a UTC hour, verifying that
  // `UTCMidnight()` does not round to the wrong hour.
  AdvanceClockToNextUTCMidnight();  // Day 0 midnight UTC.

  // Day 0, UTC 10:00. Record a search before the ping.
  task_environment_.AdvanceClock(base::Hours(10));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 0, UTC 18:15. Ping fires (local midnight in UTC+5:45).
  task_environment_.AdvanceClock(base::Hours(8) + base::Minutes(15));
  // Yesterday UTC is [Day -1 midnight, Day 0 midnight - 1ms]. No searches.
  // The UTC 10:00 search is in today's UTC day 0 bucket, not yesterday.
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
  SimulateSendingDailyUsagePingAt(base::Time::Now());

  // Day 0, UTC 21:15. Record a search after the ping, still in UTC day 0.
  task_environment_.AdvanceClock(base::Hours(3));
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1, UTC 18:15. Next ping fires (local midnight in UTC+5:45).
  task_environment_.AdvanceClock(base::Hours(21));
  // Yesterday UTC is [Day 0 midnight, Day 1 midnight - 1ms].
  // `GetStartOfStalePeriod` returns UTCMidnight(Day 0 18:15) = Day 0 midnight.
  // Both searches from Day 0 are in the Day 0 UTC bucket, within yesterday.
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

// Calendar boundary tests.

TEST_F(SerpMetricsTest,
       SearchRecordedOnDecember31UTCCountsAsYesterdayOnJanuary1) {
  base::Time dec_31;
  ASSERT_TRUE(base::Time::FromUTCString("31 Dec 2049 12:00:00", &dec_31));
  task_environment_.AdvanceClock(dec_31 - base::Time::Now());

  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Cross into Jan 1 2050 UTC.
  AdvanceClockToNextUTCMidnight();

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       SearchRecordedOnLeapDayUTCIsReportedAsYesterdayOnMarch1) {
  // 2052 is a leap year. Verifies that `GetSearchCountForYesterday` correctly
  // reads the Feb 29 UTC bucket produced by `SerpMetricsTimePeriodStorage`.
  base::Time feb_29;
  ASSERT_TRUE(base::Time::FromUTCString("29 Feb 2052 12:00:00", &feb_29));
  task_environment_.AdvanceClock(feb_29 - base::Time::Now());

  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Cross into Mar 1 2052 UTC.
  AdvanceClockToNextUTCMidnight();

  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, MigrationStaleCountUsesUTCMidnightOfLastCheckYMD) {
  // Day 0
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1. Capture UTC date string for `kLastCheckYMD`.
  const std::string ping_day_ymd = FormatUTCDateAsYMD(base::Time::Now());
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 3: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 4: Today. Ping was on Day 1 (via legacy pref).
  // Stale period = Day 1 UTC midnight through end of Day 2 = 5 searches.
  local_state_.SetString(kLastCheckYMD, ping_day_ymd);
  EXPECT_EQ(5U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, MigrationYesterdayCountUnaffectedByLastCheckYMD) {
  // Day 0: no searches
  AdvanceClockToNextUTCMidnight();

  // Day 1. Capture UTC date string for `kLastCheckYMD`.
  const std::string ping_day_ymd = FormatUTCDateAsYMD(base::Time::Now());
  AdvanceClockToNextUTCMidnight();

  // Day 2: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 3: Today. Ping was on Day 1.
  local_state_.SetString(kLastCheckYMD, ping_day_ymd);
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsTest, MigrationLastCheckedAtTakesPriorityOverLastCheckYMD) {
  // Day 0
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 3: Today.
  // `kLastReportedAt` = Day 1 midnight, so stale includes Day 1 only (1
  // search). `kLastCheckYMD` = Day 0, which would include Days 0 and 1 (2
  // searches). `kLastReportedAt` must take priority.
  const base::Time day_1_midnight =
      base::Time::Now().UTCMidnight() - base::Days(2);
  SimulateSendingDailyUsagePingAt(day_1_midnight);

  SetLastCheckYMD(base::Time::Now().UTCMidnight() - base::Days(3));

  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, MigrationFutureLastCheckYMDDropsAllSearches) {
  // Day 0: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Today.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Set `kLastCheckYMD` to tomorrow's UTC date. No `kLastReportedAt` is set.
  // The migration fallback treats this as a future UTC midnight, so both
  // yesterday and stale counts are suppressed.
  SetLastCheckYMD(base::Time::Now().UTCMidnight() + base::Days(1));

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       MigrationLastCheckYMDForTodaySuppressesBothCountsAsAlreadyReported) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today. Set `kLastCheckYMD` to today's UTC date. No `kLastReportedAt`
  // is set. The migration fallback interprets this as today's UTC midnight.
  // Since stale start (today) is after stale end (yesterday - 1 ms) and after
  // end of yesterday, both counts are 0.
  SetLastCheckYMD(base::Time::Now());

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       MigrationLastCheckYMDForYesterdayYieldsZeroStaleAndFullYesterdayCount) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today. Set `kLastCheckYMD` to yesterday's UTC date. The migration
  // fallback sets the stale boundary at yesterday's midnight, making the stale
  // range empty. All of yesterday's searches are still reported.
  SetLastCheckYMD(base::Time::Now().UTCMidnight() - base::Days(1));

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, MigrationEmptyLastCheckYMDAssumesFullTimePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today. `kLastCheckYMD` is empty and `kLastReportedAt` is null, so
  // the migration fallback returns a null time. The full time period is assumed
  // unreported.
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest, MigrationInvalidLastCheckYMDAssumesFullTimePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today. `kLastCheckYMD` cannot be parsed and `kLastReportedAt` is
  // null. The migration fallback returns a null time: full period assumed.
  local_state_.SetString(kLastCheckYMD, "not-a-valid-date");

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       MigrationLastReportedAtTakesPriorityOverInvalidLastCheckYMD) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today. `kLastReportedAt` is set to yesterday's midnight;
  // `kLastCheckYMD` holds garbage. `kLastReportedAt` must win: stale starts at
  // yesterday's midnight, so the stale range is empty and all of yesterday is
  // counted.
  const base::Time yesterday_midnight =
      base::Time::Now().UTCMidnight() - base::Days(1);
  SimulateSendingDailyUsagePingAt(yesterday_midnight);
  local_state_.SetString(kLastCheckYMD, "not-a-valid-date");

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsTest,
       MigrationLastReportedAtTakesPriorityOverEmptyLastCheckYMD) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today. `kLastReportedAt` is set to yesterday's midnight;
  // `kLastCheckYMD` is empty (was never written). `kLastReportedAt` must win.
  const base::Time yesterday_midnight =
      base::Time::Now().UTCMidnight() - base::Days(1);
  SimulateSendingDailyUsagePingAt(yesterday_midnight);

  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

}  // namespace serp_metrics
