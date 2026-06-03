/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/test/fake_serp_metrics_time_period_store.h"
#include "brave/components/serp_metrics/time_period_storage/test/scoped_timezone_for_testing.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace serp_metrics {

class SerpMetricsLastReportedAtPrefMigrationTest : public ::testing::Test {
 public:
  SerpMetricsLastReportedAtPrefMigrationTest() = default;
  ~SerpMetricsLastReportedAtPrefMigrationTest() override = default;

  void SetUp() override {
    local_state_.registry()->RegisterStringPref(kLastCheckYMD,
                                                /* Never checked */ "");
    local_state_.registry()->RegisterTimePref(prefs::kLastReportedAt,
                                              /* Never checked */ base::Time());

    serp_metrics_ = std::make_unique<SerpMetrics>(
        &local_state_, test::FakeSerpMetricsTimePeriodStoreFactory());
  }

 protected:
  void AdvanceClockToNextUTCMidnight() {
    const base::Time now = base::Time::Now();
    task_environment_.AdvanceClock(now.UTCMidnight() + base::Days(1) - now);
  }

  void SetLegacyLastCheckYMDPref(base::Time at) {
    base::Time::Exploded exploded;
    at.LocalExplode(&exploded);
    local_state_.SetString(
        kLastCheckYMD, absl::StrFormat("%04d-%02d-%02d", exploded.year,
                                       exploded.month, exploded.day_of_month));
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};

  test::ScopedTimezoneForTesting timezone_{"Europe/Paris"};

  TestingPrefServiceSimple local_state_;

  std::unique_ptr<SerpMetrics> serp_metrics_;
};

TEST_F(SerpMetricsLastReportedAtPrefMigrationTest,
       SerpMetricsCountsUsesUTCMidnightOfLegacyLastCheckYMD) {
  // Day 0
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1
  // Day 0: Stale (not counted as earlier than legacy last check time)
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  SetLegacyLastCheckYMDPref(base::Time::Now().LocalMidnight());
  AdvanceClockToNextUTCMidnight();

  // Day 1: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 3: Today
  EXPECT_EQ(5U, serp_metrics_->GetSearchCountForStalePeriod());
  EXPECT_EQ(4U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
}

TEST_F(SerpMetricsLastReportedAtPrefMigrationTest,
       LastCheckedAtPrefTakesPriorityOverLegacyLastCheckYMD) {
  // Day 0: Stale (not counted as earlier than legacy last check time)
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  SetLegacyLastCheckYMDPref(base::Time::Now().LocalMidnight());
  AdvanceClockToNextUTCMidnight();

  // Day 1: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());
  AdvanceClockToNextUTCMidnight();

  // Day 2: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 3: Today
  EXPECT_EQ(1U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsLastReportedAtPrefMigrationTest,
       LegacyLastCheckYMDForTodayDropsSearchCountsAsAlreadyReported) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today
  SetLegacyLastCheckYMDPref(base::Time::Now());

  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsLastReportedAtPrefMigrationTest,
       LegacyLastCheckYMDForYesterdayYieldsZeroStaleAndFullYesterdayCount) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  SetLegacyLastCheckYMDPref(base::Time::Now().LocalMidnight());
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsLastReportedAtPrefMigrationTest,
       EmptyLegacyLastCheckYMDAssumesFullTimePeriod) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsLastReportedAtPrefMigrationTest,
       InvalidLegacyLastCheckYMDAssumesNow) {
  local_state_.SetString(kLastCheckYMD, "InvalidDate");

  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today
  EXPECT_EQ(0U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsLastReportedAtPrefMigrationTest,
       NewLastReportedAtPrefTakesPriorityOverInvalidLegacyLastCheckYMD) {
  local_state_.SetString(kLastCheckYMD, "InvalidDate");

  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsLastReportedAtPrefMigrationTest,
       LastReportedAtTakesPriorityOverEmptyLegacyLastCheckYMD) {
  // Day 0: Stale
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextUTCMidnight();

  // Day 1: Yesterday
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  local_state_.SetTime(prefs::kLastReportedAt, base::Time::Now());
  AdvanceClockToNextUTCMidnight();

  // Day 2: Today
  EXPECT_EQ(2U,
            serp_metrics_->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, serp_metrics_->GetSearchCountForStalePeriod());
}

}  // namespace serp_metrics
