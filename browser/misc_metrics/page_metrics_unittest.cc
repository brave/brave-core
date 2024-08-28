/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/page_metrics.h"

#include <memory>

#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/search_engine_choice/search_engine_choice_service_factory.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/search_engines/search_engine_choice/search_engine_choice_service.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"
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
    builder.AddTestingFactory(BookmarkModelFactory::GetInstance(),
                              BookmarkModelFactory::GetDefaultFactory());
    builder.AddTestingFactory(
        TemplateURLServiceFactory::GetInstance(),
        base::BindRepeating(&TemplateURLServiceFactory::BuildInstanceFor));
    builder.AddTestingFactory(
        search_engines::SearchEngineChoiceServiceFactory::GetInstance(),
        search_engines::SearchEngineChoiceServiceFactory::GetDefaultFactory());

    profile_ = builder.Build();

    bookmark_model_ =
        BookmarkModelFactory::GetForBrowserContext(profile_.get());
    bookmarks::test::WaitForBookmarkModelToLoad(bookmark_model_);

    history_service_ = HistoryServiceFactory::GetForProfile(
        profile_.get(), ServiceAccessType::EXPLICIT_ACCESS);
    template_url_service_ =
        TemplateURLServiceFactory::GetForProfile(profile_.get());
    search_test_utils::WaitForTemplateURLServiceToLoad(template_url_service_);
    search_engine_choice_service_ =
        search_engines::SearchEngineChoiceServiceFactory::GetForProfile(
            profile_.get());

    misc_metrics::PageMetrics::RegisterPrefs(local_state_.registry());
    first_run_time_ = base::Time::Now();
    page_metrics_service_ = std::make_unique<PageMetrics>(
        &local_state_,
        HostContentSettingsMapFactory::GetForProfile(profile_.get()),
        history_service_, bookmark_model_, template_url_service_,
        base::BindLambdaForTesting([&]() { return first_run_time_; }));

    auto google_prepopulate_data =
        TemplateURLPrepopulateData::GetPrepopulatedEngine(
            profile_->GetPrefs(), search_engine_choice_service_,
            TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE);
    auto brave_prepopulate_data =
        TemplateURLPrepopulateData::GetPrepopulatedEngine(
            profile_->GetPrefs(), search_engine_choice_service_,
            TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE);
    google_template_url_ =
        std::make_unique<TemplateURL>(*google_prepopulate_data);
    brave_template_url_ =
        std::make_unique<TemplateURL>(*brave_prepopulate_data);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<PageMetrics> page_metrics_service_;
  raw_ptr<history::HistoryService> history_service_;
  raw_ptr<bookmarks::BookmarkModel> bookmark_model_;
  raw_ptr<TemplateURLService> template_url_service_;

  raw_ptr<search_engines::SearchEngineChoiceService>
      search_engine_choice_service_;
  std::unique_ptr<TemplateURL> google_template_url_;
  std::unique_ptr<TemplateURL> brave_template_url_;

  base::Time first_run_time_;
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
  template_url_service_->SetUserSelectedDefaultSearchProvider(
      brave_template_url_.get());

  task_environment_.FastForwardBy(base::Seconds(30));

  histogram_tester_.ExpectUniqueSample(kPagesLoadedBraveSearchHistogramName, 0,
                                       1);
  histogram_tester_.ExpectUniqueSample(kPagesReloadedHistogramName, 0, 1);

  for (size_t i = 0; i < 6; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false);
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedBraveSearchHistogramName, 1,
                                      1);
  histogram_tester_.ExpectUniqueSample(kPagesReloadedHistogramName, 0, 2);

  for (size_t i = 0; i < 30; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false);
  }
  for (size_t i = 0; i < 9; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(true);
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedBraveSearchHistogramName, 2,
                                      1);
  histogram_tester_.ExpectBucketCount(kPagesReloadedHistogramName, 1, 1);

  template_url_service_->SetUserSelectedDefaultSearchProvider(
      google_template_url_.get());

  histogram_tester_.ExpectUniqueSample(kPagesLoadedNonBraveSearchHistogramName,
                                       INT_MAX - 1, 3);

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedNonBraveSearchHistogramName,
                                      2, 1);
  histogram_tester_.ExpectBucketCount(kPagesLoadedBraveSearchHistogramName,
                                      INT_MAX - 1, 1);
  histogram_tester_.ExpectBucketCount(kPagesReloadedHistogramName, 1, 2);

  template_url_service_->SetUserSelectedDefaultSearchProvider(
      brave_template_url_.get());

  for (size_t i = 0; i < 30; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false);
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedBraveSearchHistogramName, 3,
                                      1);
  histogram_tester_.ExpectBucketCount(kPagesReloadedHistogramName, 1, 3);

  histogram_tester_.ExpectTotalCount(kPagesLoadedBraveSearchHistogramName, 5);
  histogram_tester_.ExpectTotalCount(kPagesLoadedNonBraveSearchHistogramName,
                                     5);
  histogram_tester_.ExpectTotalCount(kPagesReloadedHistogramName, 5);
  task_environment_.FastForwardBy(base::Days(7));
  EXPECT_GT(
      histogram_tester_.GetBucketCount(kPagesLoadedBraveSearchHistogramName, 0),
      1);
  EXPECT_GT(histogram_tester_.GetBucketCount(kPagesReloadedHistogramName, 0),
            1);
}

TEST_F(PageMetricsUnitTest, BookmarkCount) {
  task_environment_.FastForwardBy(base::Seconds(30));
  histogram_tester_.ExpectUniqueSample(kBookmarkCountHistogramName, 0, 1);
  bookmarks::AddIfNotBookmarked(bookmark_model_, GURL("https://example.com"),
                                u"title");

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kBookmarkCountHistogramName, 1, 1);

  for (int i = 0; i < 5; i++) {
    bookmarks::AddIfNotBookmarked(
        bookmark_model_, GURL("https://example.com/" + base::NumberToString(i)),
        u"title");
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kBookmarkCountHistogramName, 2, 1);
  histogram_tester_.ExpectTotalCount(kBookmarkCountHistogramName, 3);
}

TEST_F(PageMetricsUnitTest, FirstPageLoadTimeImmediate) {
  task_environment_.FastForwardBy(base::Minutes(1));
  histogram_tester_.ExpectTotalCount(kFirstPageLoadTimeHistogramName, 0);

  page_metrics_service_->IncrementPagesLoadedCount(false);
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 0, 1);

  task_environment_.FastForwardBy(base::Hours(2));

  page_metrics_service_->IncrementPagesLoadedCount(false);
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 0, 1);
}

TEST_F(PageMetricsUnitTest, FirstPageLoadTimeLater) {
  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectTotalCount(kFirstPageLoadTimeHistogramName, 0);

  page_metrics_service_->IncrementPagesLoadedCount(false);
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Days(2));

  page_metrics_service_->IncrementPagesLoadedCount(false);
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Days(8));
  page_metrics_service_->IncrementPagesLoadedCount(false);
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 2, 1);
}

TEST_F(PageMetricsUnitTest, FirstPageLoadTimeTooLate) {
  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kFirstPageLoadTimeHistogramName, 0);

  page_metrics_service_->IncrementPagesLoadedCount(false);
  histogram_tester_.ExpectTotalCount(kFirstPageLoadTimeHistogramName, 0);
}

}  // namespace misc_metrics
