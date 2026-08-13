/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_p3a.h"

#include <climits>
#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/serp_metrics/profile_attributes_time_period_store_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

class SerpMetricsP3ATest : public testing::Test {
 public:
  SerpMetricsP3ATest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    // Advance to a specific time so tests have a predictable starting point
    // (MOCK_TIME starts near Unix epoch, where times serialize to 0.0 and
    // deserialize back as null, causing a DCHECK in `UTCMidnight`).
    base::Time time;
    CHECK(base::Time::FromUTCString("2050-01-04 12:35:56", &time));
    task_environment_.AdvanceClock(time - base::Time::Now());

    local_state_.registry()->RegisterStringPref(kLastCheckYMD, "");
    local_state_.registry()->RegisterTimePref(prefs::kLastReportedAt,
                                              base::Time());
    local_state_.registry()->RegisterDictionaryPref(
        prefs::kP3ALastReportedAtDict);

    scoped_feature_list_.InitAndEnableFeature(kSerpMetricsFeature);

    profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(profile_manager_->SetUp());
    TestingProfile* profile = profile_manager_->CreateTestingProfile("profile");

    serp_metrics_ = std::make_unique<SerpMetrics>(
        &local_state_,
        ProfileAttributesTimePeriodStoreFactory(
            profile->GetPath(), g_browser_process->profile_manager()
                                    ->GetProfileAttributesStorage()));

    serp_metrics_p3a_ = std::make_unique<SerpMetricsP3A>(local_state_);
    serp_metrics_p3a_->Init();
  }

  void TearDown() override {
    serp_metrics_p3a_.reset();
    serp_metrics_.reset();
    profile_manager_.reset();
  }

  void AdvanceClockToNextDay() {
    task_environment_.AdvanceClock(base::Days(1));
  }

  void AdvanceClockToNextUTCMidnight() {
    const base::Time now = base::Time::Now();
    task_environment_.AdvanceClock(now.UTCMidnight() + base::Days(1) - now);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  std::unique_ptr<SerpMetrics> serp_metrics_;
  std::unique_ptr<SerpMetricsP3A> serp_metrics_p3a_;
};

TEST_F(SerpMetricsP3ATest, NothingRecordedUntilRotation) {
  histogram_tester_.ExpectTotalCount(kBraveSerpHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kGoogleSerpHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kOtherSerpHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kStaleSerpHistogramName, 0);
}

TEST_F(SerpMetricsP3ATest, SlowAndTypicalRotationsAreNoOps) {
  serp_metrics_p3a_->OnRotation(p3a::MetricLogType::kSlow);
  serp_metrics_p3a_->OnRotation(p3a::MetricLogType::kTypical);
  histogram_tester_.ExpectTotalCount(kBraveSerpHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kGoogleSerpHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kOtherSerpHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kStaleSerpHistogramName, 0);
}

TEST_F(SerpMetricsP3ATest, ReportsZerosOnExpressRotation) {
  serp_metrics_p3a_->OnRotation(p3a::MetricLogType::kExpress);
  // Brave is reported for 0+; count 0 maps to bucket 0.
  histogram_tester_.ExpectUniqueSample(kBraveSerpHistogramName, 0, 1);
  // The others are suppressed when zero.
  histogram_tester_.ExpectUniqueSample(kGoogleSerpHistogramName, INT_MAX - 1,
                                       1);
  histogram_tester_.ExpectUniqueSample(kOtherSerpHistogramName, INT_MAX - 1, 1);
  histogram_tester_.ExpectUniqueSample(kStaleSerpHistogramName, INT_MAX - 1, 1);
}

TEST_F(SerpMetricsP3ATest, ReportsAfterUTCMidnightRollover) {
  // Day 0: Yesterday — record searches for each engine.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  // Day 1: Today — advance to the next UTC midnight. Day 0 is now yesterday.
  AdvanceClockToNextUTCMidnight();
  serp_metrics_p3a_->OnRotation(p3a::MetricLogType::kExpress);

  // Brave: 1 query -> bucket 1.
  histogram_tester_.ExpectUniqueSample(kBraveSerpHistogramName, 1, 1);
  // Google: 1 query -> bucket 1.
  histogram_tester_.ExpectUniqueSample(kGoogleSerpHistogramName, 1, 1);
  // Other: 1 query -> bucket 1.
  histogram_tester_.ExpectUniqueSample(kOtherSerpHistogramName, 1, 1);
  // Stale: 0 queries -> suppressed.
  histogram_tester_.ExpectUniqueSample(kStaleSerpHistogramName, INT_MAX - 1, 1);
}

TEST_F(SerpMetricsP3ATest, TodaySearchesExcludedFromAllMetrics) {
  // Day 0: Today — record searches.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);

  // Trigger a rotation without advancing the clock — today's searches should
  // not be counted.
  serp_metrics_p3a_->OnRotation(p3a::MetricLogType::kExpress);

  // Brave: 0 today -> bucket 0.
  histogram_tester_.ExpectUniqueSample(kBraveSerpHistogramName, 0, 1);
  // Others suppressed.
  histogram_tester_.ExpectUniqueSample(kGoogleSerpHistogramName, INT_MAX - 1,
                                       1);
  histogram_tester_.ExpectUniqueSample(kOtherSerpHistogramName, INT_MAX - 1, 1);
  histogram_tester_.ExpectUniqueSample(kStaleSerpHistogramName, INT_MAX - 1, 1);
}

TEST_F(SerpMetricsP3ATest, StaleSerpSumsAcrossAllEngines) {
  // Day 0: Stale.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Yesterday (no searches).
  AdvanceClockToNextDay();

  // Day 2: Today — trigger the report.
  AdvanceClockToNextUTCMidnight();
  serp_metrics_p3a_->OnRotation(p3a::MetricLogType::kExpress);

  // 3 total queries across all engines -> bucket 3.
  histogram_tester_.ExpectUniqueSample(kStaleSerpHistogramName, 3, 1);
  histogram_tester_.ExpectUniqueSample(kBraveSerpHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kGoogleSerpHistogramName, INT_MAX - 1,
                                       1);
  histogram_tester_.ExpectUniqueSample(kOtherSerpHistogramName, INT_MAX - 1, 1);
}

TEST_F(SerpMetricsP3ATest, BucketBoundaries) {
  // Day 0: Stale — 2 Brave searches.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Yesterday — record searches at a boundary value per engine.
  for (int i = 0; i < 18; ++i) {
    serp_metrics_->RecordSearch(SerpMetricType::kBrave);
  }
  for (int i = 0; i < 35; ++i) {
    serp_metrics_->RecordSearch(SerpMetricType::kGoogle);
  }
  for (int i = 0; i < 5; ++i) {
    serp_metrics_->RecordSearch(SerpMetricType::kOther);
  }

  // Day 2: Day 1 is now yesterday.
  AdvanceClockToNextUTCMidnight();
  serp_metrics_p3a_->OnRotation(p3a::MetricLogType::kExpress);

  // Brave: 18 yesterday -> bucket 11 (18+).
  histogram_tester_.ExpectUniqueSample(kBraveSerpHistogramName, 11, 1);
  // Google: 35 yesterday -> bucket 12 (35+).
  histogram_tester_.ExpectUniqueSample(kGoogleSerpHistogramName, 12, 1);
  // Other: 5 yesterday -> bucket 5 (5+).
  histogram_tester_.ExpectUniqueSample(kOtherSerpHistogramName, 5, 1);
  // Stale: 2 from Day 0 -> bucket 2.
  histogram_tester_.ExpectUniqueSample(kStaleSerpHistogramName, 2, 1);
}

TEST_F(SerpMetricsP3ATest, OnMetricCycledUpdatesPerMetricLastReportedTime) {
  // Day 0: Record a Brave search.
  serp_metrics_->RecordSearch(SerpMetricType::kBrave);

  // Day 1: Day 0 is now yesterday.
  AdvanceClockToNextUTCMidnight();

  // First rotation reports the Brave search (yesterday = 1).
  serp_metrics_p3a_->OnRotation(p3a::MetricLogType::kExpress);
  histogram_tester_.ExpectUniqueSample(kBraveSerpHistogramName, 1, 1);

  // Cycle the Brave metric — this sets its last-reported time to now.
  serp_metrics_p3a_->OnMetricCycled(kBraveSerpHistogramName);

  // Second rotation: the cutoff now covers Day 0, so yesterday (Day 0) has
  // no unreported Brave searches. Brave: 0 -> bucket 0.
  serp_metrics_p3a_->OnRotation(p3a::MetricLogType::kExpress);
  histogram_tester_.ExpectBucketCount(kBraveSerpHistogramName, 0, 1);
  histogram_tester_.ExpectTotalCount(kBraveSerpHistogramName, 2);
}

}  // namespace serp_metrics
