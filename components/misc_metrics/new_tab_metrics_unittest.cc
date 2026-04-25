// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/misc_metrics/new_tab_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

using TemplateURLPrepopulateData::BravePrepopulatedEngineID;

class NewTabMetricsTest : public testing::Test {
 public:
  NewTabMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    NewTabMetrics::RegisterPrefs(pref_service_.registry());
    metrics_ = std::make_unique<NewTabMetrics>(&pref_service_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<NewTabMetrics> metrics_;
  base::HistogramTester histogram_tester_;
};

TEST_F(NewTabMetricsTest, ReportNTPSearchDefaultEngine) {
  metrics_->ReportNTPSearchDefaultEngine(
      BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_BRAVE);
  histogram_tester_.ExpectUniqueSample(kNTPSearchEngineHistogramName, 0, 1);

  metrics_->ReportNTPSearchDefaultEngine(std::nullopt);
  histogram_tester_.ExpectBucketCount(kNTPSearchEngineHistogramName,
                                      INT_MAX - 1, 1);

  metrics_->ReportNTPSearchDefaultEngine(
      BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_DUCKDUCKGO);
  histogram_tester_.ExpectBucketCount(kNTPSearchEngineHistogramName, 2, 1);
  histogram_tester_.ExpectTotalCount(kNTPSearchEngineHistogramName, 3);
}

TEST_F(NewTabMetricsTest, ReportNTPSearchUsage) {
  for (int i = 0; i < 10; ++i) {
    metrics_->ReportNTPSearchUsage(
        BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_BRAVE);
  }
  histogram_tester_.ExpectUniqueSample(kNTPSearchUsageHistogramName, 0, 10);
  histogram_tester_.ExpectTotalCount(kNTPSearchUsageHistogramName, 10);

  for (int i = 0; i < 5; ++i) {
    metrics_->ReportNTPSearchUsage(
        BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_BRAVE);
  }

  histogram_tester_.ExpectBucketCount(kNTPSearchUsageHistogramName, 1, 5);
  histogram_tester_.ExpectTotalCount(kNTPSearchUsageHistogramName, 15);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectBucketCount(kNTPSearchUsageHistogramName, 1, 11);
  histogram_tester_.ExpectTotalCount(kNTPSearchUsageHistogramName, 21);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kNTPSearchUsageHistogramName, 21);
}

TEST_F(NewTabMetricsTest, ReportGoogleNTPSearchUsage) {
  metrics_->ReportNTPSearchUsage(
      BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_BRAVE);
  histogram_tester_.ExpectTotalCount(kNTPGoogleWidgetUsageHistogramName, 0);
  metrics_->ReportNTPSearchUsage(
      BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_GOOGLE);
  histogram_tester_.ExpectUniqueSample(kNTPGoogleWidgetUsageHistogramName, 1,
                                       1);
}

}  // namespace misc_metrics
