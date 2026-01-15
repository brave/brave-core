/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/page_metrics.h"

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/misc_metrics/default_browser_monitor.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_MAC)
#include "base/mac/mac_util.h"
#endif

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

    profile_ = builder.Build();

    bookmark_model_ =
        BookmarkModelFactory::GetForBrowserContext(profile_.get());
    bookmarks::test::WaitForBookmarkModelToLoad(bookmark_model_);

    history_service_ = HistoryServiceFactory::GetForProfile(
        profile_.get(), ServiceAccessType::EXPLICIT_ACCESS);

    PageMetrics::RegisterPrefs(local_state_.registry());
    first_run_time_ = base::Time::Now();

#if BUILDFLAG(IS_ANDROID)
    default_browser_monitor_ = std::make_unique<DefaultBrowserMonitor>();
    default_browser_monitor_->OnDefaultBrowserStateReceived(mocked_is_default_);
#else
    default_browser_monitor_ = std::make_unique<DefaultBrowserMonitor>(
        std::make_unique<TestDelegate>(&mocked_is_default_));
    default_browser_monitor_->Start();
    task_environment_.FastForwardBy(base::Minutes(5));
#endif

    page_metrics_service_ = std::make_unique<PageMetrics>(
        &local_state_, profile_->GetPrefs(),
        HostContentSettingsMapFactory::GetForProfile(profile_.get()),
        history_service_, bookmark_model_, default_browser_monitor_.get(),
        base::BindLambdaForTesting([&]() { return first_run_time_; }));
  }

  void SetMockedDefaultBrowserStatus(bool is_default) {
    mocked_is_default_ = is_default;
    default_browser_monitor_->OnDefaultBrowserStateReceived(is_default);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<DefaultBrowserMonitor> default_browser_monitor_;
  std::unique_ptr<PageMetrics> page_metrics_service_;
  raw_ptr<history::HistoryService> history_service_;
  raw_ptr<bookmarks::BookmarkModel> bookmark_model_;

  base::Time first_run_time_;
  bool mocked_is_default_ = false;

 private:
#if !BUILDFLAG(IS_ANDROID)
  class TestDelegate : public DefaultBrowserMonitor::Delegate {
   public:
    explicit TestDelegate(bool* is_default) : is_default_(is_default) {}

    bool IsDefaultBrowser() override { return *is_default_; }
    bool IsFirstRun() override { return false; }

   private:
    raw_ptr<bool> is_default_;
  };
#endif
};

TEST_F(PageMetricsUnitTest, DomainsLoadedCount) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
  // Test with non-default browser (already set in SetUp)
  histogram_tester_.ExpectTotalCount(kDomainsLoadedNonDefaultHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kDomainsLoadedDefaultHistogramName, 0);

  task_environment_.FastForwardBy(base::Seconds(30));

  histogram_tester_.ExpectUniqueSample(kDomainsLoadedNonDefaultHistogramName, 0,
                                       1);
  histogram_tester_.ExpectUniqueSample(kDomainsLoadedDefaultHistogramName,
                                       INT_MAX - 1, 1);

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

  histogram_tester_.ExpectBucketCount(kDomainsLoadedNonDefaultHistogramName, 1,
                                      0);
  task_environment_.FastForwardBy(base::Days(1));
  EXPECT_GE(histogram_tester_.GetBucketCount(
                kDomainsLoadedNonDefaultHistogramName, 1),
            1);
  EXPECT_GE(histogram_tester_.GetBucketCount(kDomainsLoadedDefaultHistogramName,
                                             INT_MAX - 1),
            2);

  // Switch to default browser
  SetMockedDefaultBrowserStatus(true);

  history_service_->AddPage(GURL("https://aaa.com"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);
  history_service_->AddPage(GURL("https://bbb.com"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);

  histogram_tester_.ExpectBucketCount(kDomainsLoadedDefaultHistogramName, 2, 0);
  task_environment_.FastForwardBy(base::Days(1));
  EXPECT_GE(
      histogram_tester_.GetBucketCount(kDomainsLoadedDefaultHistogramName, 2),
      1);
  EXPECT_GE(histogram_tester_.GetBucketCount(
                kDomainsLoadedNonDefaultHistogramName, INT_MAX - 1),
            1);

  int init_zero_count =
      histogram_tester_.GetBucketCount(kDomainsLoadedDefaultHistogramName, 0);
  task_environment_.FastForwardBy(base::Days(7));
  EXPECT_GT(
      histogram_tester_.GetBucketCount(kDomainsLoadedDefaultHistogramName, 0),
      init_zero_count);
}

TEST_F(PageMetricsUnitTest, PagesLoadedCount) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
  // Set up rewards status
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);

  task_environment_.FastForwardBy(base::Seconds(30));

  histogram_tester_.ExpectUniqueSample(kPagesLoadedNonRewardsHistogramName, 0,
                                       1);
  histogram_tester_.ExpectUniqueSample(kPagesReloadedHistogramName, 0, 1);

  for (size_t i = 0; i < 6; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false);
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedNonRewardsHistogramName, 1,
                                      1);
  histogram_tester_.ExpectUniqueSample(kPagesReloadedHistogramName, 0, 2);

  for (size_t i = 0; i < 30; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false);
  }
  for (size_t i = 0; i < 9; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(true);
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedNonRewardsHistogramName, 2,
                                      1);
  histogram_tester_.ExpectBucketCount(kPagesReloadedHistogramName, 1, 1);

  // Change to Rewards enabled
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  histogram_tester_.ExpectBucketCount(kPagesLoadedNonRewardsHistogramName,
                                      INT_MAX - 1, 1);
  histogram_tester_.ExpectBucketCount(kPagesLoadedRewardsHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedRewardsHistogramName, 2, 2);
  histogram_tester_.ExpectBucketCount(kPagesReloadedHistogramName, 1, 3);

  // Change to Rewards with wallet
  profile_->GetPrefs()->SetString(brave_rewards::prefs::kExternalWalletType,
                                  "uphold");

  for (size_t i = 0; i < 30; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false);
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedRewardsWalletHistogramName, 3,
                                      1);
  histogram_tester_.ExpectBucketCount(kPagesReloadedHistogramName, 1, 5);

  histogram_tester_.ExpectTotalCount(kPagesLoadedNonRewardsHistogramName, 7);
  histogram_tester_.ExpectTotalCount(kPagesLoadedRewardsHistogramName, 7);
  histogram_tester_.ExpectTotalCount(kPagesLoadedRewardsWalletHistogramName, 7);
  histogram_tester_.ExpectTotalCount(kPagesReloadedHistogramName, 7);
  task_environment_.FastForwardBy(base::Days(7));
  EXPECT_GT(histogram_tester_.GetBucketCount(
                kPagesLoadedRewardsWalletHistogramName, 0),
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
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Hours(2));

  page_metrics_service_->IncrementPagesLoadedCount(false);
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 1, 1);
}

TEST_F(PageMetricsUnitTest, FirstPageLoadTimeLater) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
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
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() == 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe.";
  }
#endif
  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kFirstPageLoadTimeHistogramName, 0);

  page_metrics_service_->IncrementPagesLoadedCount(false);
  histogram_tester_.ExpectTotalCount(kFirstPageLoadTimeHistogramName, 0);
}

TEST_F(PageMetricsUnitTest, BraveSearchDaily) {
  // Test with non-default browser
  page_metrics_service_->ReportBraveQuery();
  histogram_tester_.ExpectUniqueSample(kSearchBraveDailyHistogramName, false,
                                       1);

  // Switch to default browser
  SetMockedDefaultBrowserStatus(true);

  page_metrics_service_->ReportBraveQuery();
  histogram_tester_.ExpectBucketCount(kSearchBraveDailyHistogramName, true, 1);
  histogram_tester_.ExpectTotalCount(kSearchBraveDailyHistogramName, 2);
}

}  // namespace misc_metrics
