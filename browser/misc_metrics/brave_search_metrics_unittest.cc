/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/brave_search_metrics.h"

#include <memory>

#include "base/test/bind.h"
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
  // Set Brave as default engine.
  SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_search);

  // Record 1 query -> bucket 1 (1-3 range)
  brave_search_metrics_->RecordBraveQuery();
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesBraveDefaultHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesGoogleDefaultHistogramName, INT_MAX - 1, 1);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesOtherDefaultHistogramName, INT_MAX - 1, 1);

  // Record 2 more (total 3) -> still bucket 1
  brave_search_metrics_->RecordBraveQuery();
  brave_search_metrics_->RecordBraveQuery();
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 1, 3);

  // Record 1 more (total 4) -> bucket 2
  brave_search_metrics_->RecordBraveQuery();
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 2, 1);

  // Record 3 more (total 7) -> still bucket 2
  for (int i = 0; i < 3; i++) {
    brave_search_metrics_->RecordBraveQuery();
  }
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 2, 4);

  // Record 1 more (total 8) -> bucket 3
  brave_search_metrics_->RecordBraveQuery();
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 3, 1);

  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 8);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesGoogleDefaultHistogramName, 8);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesOtherDefaultHistogramName, 8);
}

TEST_F(BraveSearchMetricsUnitTest, DailyQueriesEngineSwitch) {
  // Set Google as default engine
  SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_google);

  brave_search_metrics_->RecordBraveQuery();
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesGoogleDefaultHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesBraveDefaultHistogramName, INT_MAX - 1, 1);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesOtherDefaultHistogramName, INT_MAX - 1, 1);

  // Record more queries under Google default
  for (int i = 0; i < 3; i++) {
    brave_search_metrics_->RecordBraveQuery();
  }
  // Total is now 4 -> bucket 2
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesGoogleDefaultHistogramName, 2, 1);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesBraveDefaultHistogramName, INT_MAX - 1, 4);
  histogram_tester_.ExpectUniqueSample(
      kSearchDailyQueriesOtherDefaultHistogramName, INT_MAX - 1, 4);

  // Switch to Bing (OtherDefault)
  SetDefaultSearchEngine(TemplateURLPrepopulateData::brave_bing);

  // After switch, OtherDefault should be active with current count (4 queries),
  // Google should be invalidated.
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesOtherDefaultHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(
      kSearchDailyQueriesGoogleDefaultHistogramName, INT_MAX - 1, 1);

  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 6);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesGoogleDefaultHistogramName, 6);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesOtherDefaultHistogramName, 6);
}

TEST_F(BraveSearchMetricsUnitTest, DailyQueriesNoReportOnZero) {
  // No queries recorded, nothing should be reported.
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesBraveDefaultHistogramName, 0);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesGoogleDefaultHistogramName, 0);
  histogram_tester_.ExpectTotalCount(
      kSearchDailyQueriesOtherDefaultHistogramName, 0);
}

}  // namespace misc_metrics
