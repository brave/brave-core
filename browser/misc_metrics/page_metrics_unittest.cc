/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/misc_metrics/page_metrics.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class PageMetricsUnitTest : public testing::Test {
 public:
  PageMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    TestingProfile::Builder builder;
    builder.AddTestingFactory(HistoryServiceFactory::GetInstance(),
                              HistoryServiceFactory::GetDefaultFactory());
    profile_ = builder.Build();

    history_service_ = HistoryServiceFactory::GetForProfile(
        profile_.get(), ServiceAccessType::EXPLICIT_ACCESS);
    misc_metrics::PageMetrics::RegisterPrefs(local_state_.registry());
    page_metrics_service_ =
        std::make_unique<PageMetrics>(&local_state_, history_service_);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<PageMetrics> page_metrics_service_;
  raw_ptr<history::HistoryService> history_service_;
};

TEST_F(PageMetricsUnitTest, DomainsLoadedCount) {
  histogram_tester_.ExpectTotalCount(kDomainsLoadedHistogramName, 0);

  task_environment_.FastForwardBy(base::Seconds(30));

  histogram_tester_.ExpectUniqueSample(kDomainsLoadedHistogramName, 0, 1);

  history_service_->AddPage(GURL("https://abc.com"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);
  history_service_->AddPage(GURL("https://def.org"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);
  history_service_->AddPage(GURL("https://xyz.org"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);
  history_service_->AddPage(GURL("https://xyz.net/page1"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);
  history_service_->AddPage(GURL("https://xyz.net/page2"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);

  histogram_tester_.ExpectBucketCount(kDomainsLoadedHistogramName, 1, 0);
  task_environment_.FastForwardBy(base::Days(1));
  EXPECT_GE(histogram_tester_.GetBucketCount(kDomainsLoadedHistogramName, 1),
            1);

  history_service_->AddPage(GURL("https://aaa.com"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);
  history_service_->AddPage(GURL("https://bbb.com"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);

  histogram_tester_.ExpectBucketCount(kDomainsLoadedHistogramName, 2, 0);
  task_environment_.FastForwardBy(base::Days(1));
  EXPECT_GE(histogram_tester_.GetBucketCount(kDomainsLoadedHistogramName, 2),
            1);

  int init_zero_count =
      histogram_tester_.GetBucketCount(kDomainsLoadedHistogramName, 0);
  task_environment_.FastForwardBy(base::Days(7));
  EXPECT_GT(histogram_tester_.GetBucketCount(kDomainsLoadedHistogramName, 0),
            init_zero_count);
}

TEST_F(PageMetricsUnitTest, PagesLoadedCount) {
  task_environment_.FastForwardBy(base::Seconds(30));

  histogram_tester_.ExpectUniqueSample(kPagesLoadedHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kPagesReloadedHistogramName, 0, 1);

  for (size_t i = 0; i < 6; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false);
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kPagesReloadedHistogramName, 0, 2);

  for (size_t i = 0; i < 30; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false);
  }
  for (size_t i = 0; i < 9; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(true);
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(kPagesReloadedHistogramName, 1, 1);

  for (size_t i = 0; i < 30; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false);
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedHistogramName, 3, 1);
  histogram_tester_.ExpectBucketCount(kPagesReloadedHistogramName, 1, 2);

  histogram_tester_.ExpectTotalCount(kPagesLoadedHistogramName, 4);
  histogram_tester_.ExpectTotalCount(kPagesReloadedHistogramName, 4);
  task_environment_.FastForwardBy(base::Days(7));
  EXPECT_GT(histogram_tester_.GetBucketCount(kPagesLoadedHistogramName, 0), 1);
  EXPECT_GT(histogram_tester_.GetBucketCount(kPagesReloadedHistogramName, 0),
            1);
}

}  // namespace misc_metrics
