/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/brave_search_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
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
  content::BrowserTaskEnvironment task_environment_;
  search_engines::SearchEnginesTestEnvironment search_engines_test_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<BraveSearchMetrics> brave_search_metrics_;
};

TEST_F(BraveSearchMetricsUnitTest, DailyQueriesBuckets) {
  SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_search);

  // Record 3 queries within the 24-hour window.
  for (int i = 0; i < 3; i++) {
    brave_search_metrics_->RecordBraveQuery();
  }

  // Nothing reported yet since 24 hours haven't elapsed.
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 0);

  // Advance past 24 hours. The hourly timer will trigger the report.
  task_environment_.FastForwardBy(base::Hours(24));

  // Should report bucket 1 (3 queries -> 1-3 range).
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesBraveDefaultHistogramName, 1, 1);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesGoogleDefaultHistogramName, 0);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesOtherDefaultHistogramName, 0);

  // Record 7 queries in the new window.
  for (int i = 0; i < 7; i++) {
    brave_search_metrics_->RecordBraveQuery();
  }

  task_environment_.FastForwardBy(base::Hours(24));

  // Should report bucket 2 (7 queries -> 4-7 range).
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 2, 1);

  // Record 8 queries in the new window.
  for (int i = 0; i < 8; i++) {
    brave_search_metrics_->RecordBraveQuery();
  }

  task_environment_.FastForwardBy(base::Hours(24));

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
    brave_search_metrics_->RecordBraveQuery();
  }

  // Advance past 24 hours. The hourly timer will trigger the report.
  task_environment_.FastForwardBy(base::Hours(24));

  // Should report under Google (3 queries -> bucket 1).
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesGoogleDefaultHistogramName, 1, 1);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 0);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesOtherDefaultHistogramName, 0);

  // Switch to Bing (OtherDefault).
  SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_bing);

  // Record 5 queries under Bing default.
  for (int i = 0; i < 5; i++) {
    brave_search_metrics_->RecordBraveQuery();
  }

  // Advance past 24 hours.
  task_environment_.FastForwardBy(base::Hours(24));

  // Should report under OtherDefault (5 queries -> bucket 2).
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesOtherDefaultHistogramName, 2, 1);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 0);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesGoogleDefaultHistogramName, 1, 1);

  // Switch to Brave Search.
  SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_search);

  // Record 8 queries under Brave default.
  for (int i = 0; i < 8; i++) {
    brave_search_metrics_->RecordBraveQuery();
  }

  // Advance past 24 hours.
  task_environment_.FastForwardBy(base::Hours(24));

  // Should report under BraveDefault (8 queries -> bucket 3).
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesBraveDefaultHistogramName, 3, 1);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesOtherDefaultHistogramName, 2, 1);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesGoogleDefaultHistogramName, 1, 1);
}

TEST_F(BraveSearchMetricsUnitTest, CountsClearedAfterReport) {
  SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_search);

  brave_search_metrics_->RecordBraveQuery();

  // Advance past 24 hours. The hourly timer will trigger the report.
  task_environment_.FastForwardBy(base::Hours(24));

  // Previous window reported 1 query -> bucket 1.
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 1, 1);

  // Record 1 query in the new window.
  brave_search_metrics_->RecordBraveQuery();

  // Advance another 24 hours.
  task_environment_.FastForwardBy(base::Hours(24));

  // Should report 1 query again for the new window.
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 1, 2);
}

TEST_F(BraveSearchMetricsUnitTest, NoReportBeforeFrameExpires) {
  SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_search);

  // Record queries within the 24-hour window.
  for (int i = 0; i < 5; i++) {
    brave_search_metrics_->RecordBraveQuery();
  }

  // Advance only 12 hours - should not trigger a report.
  task_environment_.FastForwardBy(base::Hours(12));

  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 0);
}

}  // namespace misc_metrics
