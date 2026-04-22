/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/test/fake_serp_metrics_time_period_store.h"
#include "brave/components/serp_metrics/time_period_storage/test/scoped_timezone_for_testing.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

namespace {

// 24 hours covers a standard day; the extra 2 hours covers the maximum DST
// shift (Antarctica/Troll), guaranteeing the result lands inside the next
// calendar day without overshooting into the day after.
constexpr base::TimeDelta kNextLocalMidnightOffset =
    base::Hours(24) + base::Hours(2);

base::Time NextLocalMidnight(base::Time time) {
  return (time + kNextLocalMidnightOffset).LocalMidnight();
}

}  // namespace

class SerpMetricsTravelTest : public testing::Test {
 public:
  SerpMetricsTravelTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    local_state_.registry()->RegisterTimePref(prefs::kLastReportedAt,
                                              base::Time());

    serp_metrics_ = std::make_unique<SerpMetrics>(
        &local_state_, test::FakeSerpMetricsTimePeriodStoreFactory());
  }

  void AdvanceClockToNextLocalMidnight() {
    const base::Time now = base::Time::Now();
    task_environment_.AdvanceClock(NextLocalMidnight(now) - now);
  }

  void AdvanceClockToNextUTCMidnight() {
    const base::Time now = base::Time::Now();
    task_environment_.AdvanceClock(now.UTCMidnight() + base::Days(1) - now);
  }

 protected:
  base::test::TaskEnvironment task_environment_;

  TestingPrefServiceSimple local_state_;

  std::unique_ptr<SerpMetrics> serp_metrics_;
};

TEST_F(SerpMetricsTravelTest,
       SearchCountsCorrectAfterLocalTimePingAndEastToWestTravel) {
  {
    const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Kiritimati");
    AdvanceClockToNextLocalMidnight();
    // Ping fires at local midnight and sets `kLastReportedAt` value.
    local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());

    AdvanceClockToNextUTCMidnight();

    // Day 0: Stale
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();
  }

  {
    const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Baker");

    // Day 1: Yesterday
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();

    // Day 2: Today
    EXPECT_EQ(
        2U, serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
    EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
  }
}

TEST_F(
    SerpMetricsTravelTest,
    SearchCountsCorrectAfterLocalTimePingWhenTravelingFromExtremeWestToExtremeEast) {
  {
    const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Baker");
    AdvanceClockToNextLocalMidnight();
    // Ping fires at local midnight and sets `kLastReportedAt` value.
    local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());

    AdvanceClockToNextUTCMidnight();

    // Day 0: Stale
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();
  }

  {
    const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Kiritimati");

    // Day 1: Yesterday
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();

    // Day 2: Today
    EXPECT_EQ(
        2U, serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
    EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
  }
}

TEST_F(SerpMetricsTravelTest,
       SearchCountsCorrectAfterLocalTimePingWhenTravelingFromUTCToExtremeWest) {
  {
    const test::ScopedTimezoneForTesting scoped_timezone("UTC");
    AdvanceClockToNextLocalMidnight();
    // Ping fires at local midnight and sets `kLastReportedAt` value.
    local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());

    AdvanceClockToNextUTCMidnight();

    // Day 0: Stale
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();
  }

  {
    const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Baker");

    // Day 1: Yesterday
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();

    // Day 2: Today
    EXPECT_EQ(
        2U, serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
    EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
  }
}

TEST_F(SerpMetricsTravelTest,
       SearchCountsCorrectAfterLocalTimePingWhenTravelingFromExtremeWestToUTC) {
  {
    const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Baker");
    AdvanceClockToNextLocalMidnight();
    // Ping fires at local midnight and sets `kLastReportedAt` value.
    local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());

    AdvanceClockToNextUTCMidnight();

    // Day 0: Stale
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();
  }

  {
    const test::ScopedTimezoneForTesting scoped_timezone("UTC");

    // Day 1: Yesterday
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();

    // Day 2: Today
    EXPECT_EQ(
        2U, serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
    EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
  }
}

TEST_F(SerpMetricsTravelTest,
       SearchCountsCorrectAfterLocalTimePingWhenTravelingFromUTCToExtremeEast) {
  {
    const test::ScopedTimezoneForTesting scoped_timezone("UTC");
    AdvanceClockToNextLocalMidnight();
    // Ping fires at local midnight and sets `kLastReportedAt` value.
    local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());

    AdvanceClockToNextUTCMidnight();

    // Day 0: Stale
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();
  }

  {
    const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Kiritimati");

    // Day 1: Yesterday
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();

    // Day 2: Today
    EXPECT_EQ(
        2U, serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
    EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
  }
}

TEST_F(SerpMetricsTravelTest,
       SearchCountsCorrectAfterLocalTimePingWhenTravelingFromExtremeEastToUTC) {
  {
    const test::ScopedTimezoneForTesting scoped_timezone("Pacific/Kiritimati");
    AdvanceClockToNextLocalMidnight();
    // Ping fires at local midnight and sets `kLastReportedAt` value.
    local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());

    AdvanceClockToNextUTCMidnight();

    // Day 0: Stale
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();
  }

  {
    const test::ScopedTimezoneForTesting scoped_timezone("UTC");

    // Day 1: Yesterday
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
    AdvanceClockToNextUTCMidnight();

    // Day 2: Today
    EXPECT_EQ(
        2U, serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
    EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
  }
}

}  // namespace serp_metrics
