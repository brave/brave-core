/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/brave_search_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "build/build_config.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/search_engines_test_environment.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class BraveSearchMetricsUnitTest : public testing::Test {
 public:
  BraveSearchMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    TemplateURLService* template_url_service =
        search_engines_test_environment_.template_url_service();
    ASSERT_TRUE(template_url_service);
    template_url_service->Load();
    ASSERT_TRUE(template_url_service->loaded());

    BraveSearchMetrics::RegisterPrefs(local_state_.registry());

    brave_search_metrics_ = std::make_unique<BraveSearchMetrics>(
        &local_state_, template_url_service);

    SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_search);
  }

  void SetDefaultSearchEngine(
      const TemplateURLPrepopulateData::PrepopulatedEngine& engine) {
    TemplateURLService* template_url_service =
        search_engines_test_environment_.template_url_service();
    auto data = TemplateURLDataFromPrepopulatedEngine(engine);
    auto* added =
        template_url_service->Add(std::make_unique<TemplateURL>(*data));
    ASSERT_TRUE(added);
    template_url_service->SetUserSelectedDefaultSearchProvider(added);
  }

 protected:
  void FastForwardAndReport(base::TimeDelta delta) {
    task_environment_.FastForwardBy(delta);
    brave_search_metrics_->ReportAllMetrics();
  }

  content::BrowserTaskEnvironment task_environment_;
  search_engines::SearchEnginesTestEnvironment search_engines_test_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<BraveSearchMetrics> brave_search_metrics_;

  const GURL brave_search_url_{"https://search.brave.com/search?q=test"};
  const GURL empty_url_;
};

TEST_F(BraveSearchMetricsUnitTest, DailyQueriesBuckets) {
  // Record 3 queries within the 24-hour window.
  for (int i = 0; i < 3; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }

  // Nothing reported yet since 24 hours haven't elapsed.
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 0);
  // Advance 24 hours then report.
  FastForwardAndReport(base::Days(1));

  // Should report bucket 1 (3 queries -> 1-3 range).
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesBraveDefaultHistogramName, 1, 1);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesGoogleDefaultHistogramName, 0);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesOtherDefaultHistogramName, 0);

  // Record 7 queries in the new window.
  for (int i = 0; i < 7; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }

  FastForwardAndReport(base::Days(1));

  // Should report bucket 2 (7 queries -> 4-7 range).
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 2, 1);

  // Record 8 queries in the new window.
  for (int i = 0; i < 8; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }

  FastForwardAndReport(base::Days(1));

  // Should report bucket 3 (8 queries -> 8+ range).
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 3, 1);

  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 3);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesGoogleDefaultHistogramName, 0);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesOtherDefaultHistogramName, 0);
}

TEST_F(BraveSearchMetricsUnitTest, DailyQueriesEngineSwitch) {
  // Start with Google as default.
  SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_google);

  // Record 3 queries under Google default.
  for (int i = 0; i < 3; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }

  // Advance past 24 hours then report.
  FastForwardAndReport(base::Days(1));

  // Should report under Google (3 queries -> bucket 1).
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesGoogleDefaultHistogramName, 1, 1);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kSearchDailyQueriesDDGDefaultHistogramName,
                                     0);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesYahooDefaultHistogramName, 0);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesOtherDefaultHistogramName, 0);

  // Switch to Bing (OtherDefault).
  SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_bing);

  // Record 5 queries under Bing default.
  for (int i = 0; i < 5; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }

  // Advance past 24 hours then report.
  FastForwardAndReport(base::Days(1));

  // Should report under OtherDefault (5 queries -> bucket 2).
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesOtherDefaultHistogramName, 2, 1);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 0);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesGoogleDefaultHistogramName, 1, 1);
  histogram_tester_.ExpectTotalCount(kSearchDailyQueriesDDGDefaultHistogramName,
                                     0);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesYahooDefaultHistogramName, 0);

  // Switch to Brave Search.
  SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_search);

  // Record 8 queries under Brave default.
  for (int i = 0; i < 8; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }

  // Advance past 24 hours then report.
  FastForwardAndReport(base::Days(1));

  // Should report under BraveDefault (8 queries -> bucket 3).
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesBraveDefaultHistogramName, 3, 1);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesOtherDefaultHistogramName, 2, 1);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesGoogleDefaultHistogramName, 1, 1);
  histogram_tester_.ExpectTotalCount(kSearchDailyQueriesDDGDefaultHistogramName,
                                     0);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesYahooDefaultHistogramName, 0);
}

TEST_F(BraveSearchMetricsUnitTest, CountsClearedAfterReport) {
  brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);

  // Advance past 24 hours then report.
  FastForwardAndReport(base::Days(1));

  // Previous window reported 1 query -> bucket 1.
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 1, 1);

  // Record 1 query in the new window.
  brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);

  // Advance another 24 hours then report.
  FastForwardAndReport(base::Days(1));

  // Should report 1 query again for the new window.
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 1, 2);
}

TEST_F(BraveSearchMetricsUnitTest, ClearQueryCounts) {
  for (int i = 0; i < 5; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }

  brave_search_metrics_->ClearQueryCounts();

  // No report yet before the frame expires.
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 0);

  // Advance past 24 hours and report.
  FastForwardAndReport(base::Days(1));

  // Should report bucket 0 (0 queries) since counts were cleared.
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesBraveDefaultHistogramName, 0, 1);
}

TEST_F(BraveSearchMetricsUnitTest, NoReportBeforeFrameExpires) {
  // Record queries within the 24-hour window.
  for (int i = 0; i < 5; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }

  // Only 12 hours have elapsed - should not report.
  FastForwardAndReport(base::Hours(12));

  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 0);
}

TEST_F(BraveSearchMetricsUnitTest, OmniboxEntryPercentages) {
  // Simulate 100 primary queries:
  // - 90 from omnibox typed (90%)
  // - 10 from omnibox suggestions (10%)
  for (int i = 0; i < 100; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }
  for (int i = 0; i < 90; i++) {
    brave_search_metrics_->MaybeRecordOmniboxQuery(brave_search_url_, false);
  }
  for (int i = 0; i < 10; i++) {
    brave_search_metrics_->MaybeRecordOmniboxQuery(brave_search_url_, true);
  }

  FastForwardAndReport(base::Days(1));

  // 90% typed -> bucket 4 (81-95%)
  histogram_tester_.ExpectUniqueSample(kSearchOmniboxTypedPercentHistogramName,
                                       4, 1);
  // 10% suggestion -> bucket 2 (6-20%)
  histogram_tester_.ExpectUniqueSample(
      kSearchOmniboxSuggestionPercentHistogramName, 2, 1);
}

TEST_F(BraveSearchMetricsUnitTest, NTPSearchPercentage) {
  // Simulate 100 primary queries:
  // - 25 from NTP search (25%)
  for (int i = 0; i < 100; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }
  for (int i = 0; i < 25; i++) {
    brave_search_metrics_->MaybeRecordNTPSearch(static_cast<int64_t>(
        TemplateURLPrepopulateData::BravePrepopulatedEngineID::
            PREPOPULATED_ENGINE_ID_BRAVE));
  }

  FastForwardAndReport(base::Days(1));

  // 25% NTP -> bucket 3 (21-80%)
  histogram_tester_.ExpectUniqueSample(kSearchNTPSearchPercentHistogramName, 3,
                                       1);
}

TEST_F(BraveSearchMetricsUnitTest, NTPSearchNonBraveEngineIgnored) {
  // Simulate 100 primary queries with non-Brave engines
  for (int i = 0; i < 100; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }
  // Record NTP searches with Google (should be ignored)
  for (int i = 0; i < 50; i++) {
    brave_search_metrics_->MaybeRecordNTPSearch(static_cast<int64_t>(
        TemplateURLPrepopulateData::BravePrepopulatedEngineID::
            PREPOPULATED_ENGINE_ID_GOOGLE));
  }
  // Record NTP searches with Brave (should be counted)
  for (int i = 0; i < 10; i++) {
    brave_search_metrics_->MaybeRecordNTPSearch(static_cast<int64_t>(
        TemplateURLPrepopulateData::BravePrepopulatedEngineID::
            PREPOPULATED_ENGINE_ID_BRAVE));
  }

  FastForwardAndReport(base::Days(1));

  // 10% NTP (only Brave counted) -> bucket 2 (6-20%)
  histogram_tester_.ExpectUniqueSample(kSearchNTPSearchPercentHistogramName, 2,
                                       1);
}

#if BUILDFLAG(IS_ANDROID)
TEST_F(BraveSearchMetricsUnitTest, WidgetSearchPercentage) {
  const GURL non_brave_url{"https://google.com/search?q=test"};

  // Non-Brave URL should not be recorded.
  for (int i = 0; i < 100; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }
  brave_search_metrics_->MaybeRecordWidgetSearch(non_brave_url);

  FastForwardAndReport(base::Days(1));

  histogram_tester_.ExpectTotalCount(kSearchWidgetSearchPercentHistogramName,
                                     0);

  // Simulate 100 primary queries with 25 widget searches (25%).
  for (int i = 0; i < 100; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }
  for (int i = 0; i < 25; i++) {
    brave_search_metrics_->MaybeRecordWidgetSearch(brave_search_url_);
  }

  FastForwardAndReport(base::Days(1));

  // 25% widget -> bucket 3 (21-80%)
  histogram_tester_.ExpectUniqueSample(kSearchWidgetSearchPercentHistogramName,
                                       3, 1);
}
TEST_F(BraveSearchMetricsUnitTest, QuickSearchPercentage) {
  // Leo search and non-Brave keyword should not be recorded.
  for (int i = 0; i < 100; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }
  brave_search_metrics_->MaybeRecordQuickSearch(true, "");
  brave_search_metrics_->MaybeRecordQuickSearch(false, ":go");

  FastForwardAndReport(base::Days(1));

  histogram_tester_.ExpectTotalCount(kSearchQuickSearchPercentHistogramName, 0);

  // Simulate 100 primary queries with 25 quick searches (25%).
  for (int i = 0; i < 100; i++) {
    brave_search_metrics_->MaybeRecordBraveQuery(empty_url_, brave_search_url_);
  }
  for (int i = 0; i < 25; i++) {
    brave_search_metrics_->MaybeRecordQuickSearch(false, ":br");
  }

  FastForwardAndReport(base::Days(1));

  // 25% quick search -> bucket 3 (21-80%)
  histogram_tester_.ExpectUniqueSample(kSearchQuickSearchPercentHistogramName,
                                       3, 1);
}
#endif  // BUILDFLAG(IS_ANDROID)

}  // namespace misc_metrics
