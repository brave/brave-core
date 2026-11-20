/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/page_metrics.h"

#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/observer_list.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/misc_metrics/default_browser_monitor.h"
#include "brave/components/misc_metrics/media_session_metrics.h"
#include "brave/components/p3a_utils/test_event_relay_observer.h"
#include "build/build_config.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/search_engines_test_environment.h"
#include "components/search_engines/template_url_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_MAC)
#include "base/mac/mac_util.h"
#endif

namespace misc_metrics {

class PageMetricsUnitTest : public testing::Test {
 public:
  PageMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    profile_prefs_.registry()->RegisterBooleanPref(
        brave_rewards::prefs::kEnabled, false);
    profile_prefs_.registry()->RegisterStringPref(
        brave_rewards::prefs::kExternalWalletType, "");
    HostContentSettingsMap::RegisterProfilePrefs(profile_prefs_.registry());
    host_content_settings_map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &profile_prefs_, false /* is_off_the_record */,
        false /* store_last_modified */, false /* restore_session */,
        false /* should_record_metrics */);

    ASSERT_TRUE(history_dir_.CreateUniqueTempDir());
    history_service_ =
        history::CreateHistoryService(history_dir_.GetPath(), true);
    ASSERT_TRUE(history_service_);

    bookmark_model_ = bookmarks::TestBookmarkClient::CreateModel();

    PageMetrics::RegisterPrefs(local_state_.registry());
    first_run_time_ = base::Time::Now();

#if BUILDFLAG(IS_ANDROID)
    default_browser_monitor_ = std::make_unique<DefaultBrowserMonitor>();
    default_browser_monitor_->OnDefaultBrowserStateReceived(mocked_is_default_);
#else
    default_browser_monitor_ = std::make_unique<DefaultBrowserMonitor>(
        std::make_unique<TestDelegate>(&mocked_is_default_));
    default_browser_monitor_->Start();
#endif
    task_environment_.FastForwardBy(base::Minutes(5));

    page_metrics_service_ = std::make_unique<PageMetrics>(
        &local_state_, &profile_prefs_, host_content_settings_map_.get(),
        history_service_.get(), bookmark_model_.get(),
        default_browser_monitor_.get(), &media_session_metrics_,
        search_engines_test_environment_.template_url_service(),
        base::BindLambdaForTesting([&]() { return first_run_time_; }));
  }

  void TearDown() override {
    page_metrics_service_.reset();
    host_content_settings_map_->ShutdownOnUIThread();
  }

  void SetMockedDefaultBrowserStatus(bool is_default) {
    mocked_is_default_ = is_default;
    default_browser_monitor_->OnDefaultBrowserStateReceived(is_default);
  }

 protected:
  class MockMediaSessionMetrics : public MediaSessionMetrics {
   public:
    MockMediaSessionMetrics() = default;
    ~MockMediaSessionMetrics() override = default;

    MockMediaSessionMetrics(const MockMediaSessionMetrics&) = delete;
    MockMediaSessionMetrics& operator=(const MockMediaSessionMetrics&) = delete;

    // Notifies observers as if |tick_duration| of media playback just
    // occurred.
    void NotifyMediaPlaybackTick(base::TimeDelta tick_duration) {
      observers_.Notify(&Observer::OnMediaPlaybackTick, tick_duration);
    }

    // MediaSessionMetrics:
    void AddObserver(Observer* observer) override {
      observers_.AddObserver(observer);
    }
    void RemoveObserver(Observer* observer) override {
      observers_.RemoveObserver(observer);
    }

   private:
    base::ObserverList<Observer> observers_;
  };

  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  base::HistogramTester histogram_tester_;
  p3a_utils::TestEventRelayObserver relay_observer_;
  base::ScopedTempDir history_dir_;
  search_engines::SearchEnginesTestEnvironment search_engines_test_environment_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  std::unique_ptr<history::HistoryService> history_service_;
  std::unique_ptr<bookmarks::BookmarkModel> bookmark_model_;
  std::unique_ptr<DefaultBrowserMonitor> default_browser_monitor_;
  MockMediaSessionMetrics media_session_metrics_;
  std::unique_ptr<PageMetrics> page_metrics_service_;

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
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
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
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  // Set up rewards status
  profile_prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);

  task_environment_.FastForwardBy(base::Seconds(30));

  histogram_tester_.ExpectUniqueSample(kPagesLoadedNonRewardsHistogramName, 0,
                                       1);
  histogram_tester_.ExpectUniqueSample(kPagesReloadedHistogramName, 0, 1);

  for (size_t i = 0; i < 6; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false, false);
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedNonRewardsHistogramName, 1,
                                      1);
  histogram_tester_.ExpectUniqueSample(kPagesReloadedHistogramName, 0, 2);

  for (size_t i = 0; i < 30; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false, false);
  }
  for (size_t i = 0; i < 9; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(true, false);
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedNonRewardsHistogramName, 2,
                                      1);
  histogram_tester_.ExpectBucketCount(kPagesReloadedHistogramName, 1, 1);

  // Change to Rewards enabled
  profile_prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);

  histogram_tester_.ExpectBucketCount(kPagesLoadedNonRewardsHistogramName,
                                      INT_MAX - 1, 1);
  histogram_tester_.ExpectBucketCount(kPagesLoadedRewardsHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kPagesLoadedRewardsHistogramName, 2, 2);
  histogram_tester_.ExpectBucketCount(kPagesReloadedHistogramName, 1, 3);

  // Change to Rewards with wallet
  profile_prefs_.SetString(brave_rewards::prefs::kExternalWalletType, "uphold");

  for (size_t i = 0; i < 30; i++) {
    page_metrics_service_->IncrementPagesLoadedCount(false, false);
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
  bookmarks::AddIfNotBookmarked(bookmark_model_.get(),
                                GURL("https://example.com"), u"title");

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kBookmarkCountHistogramName, 1, 1);

  for (int i = 0; i < 5; i++) {
    bookmarks::AddIfNotBookmarked(
        bookmark_model_.get(),
        GURL("https://example.com/" + base::NumberToString(i)), u"title");
  }

  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectBucketCount(kBookmarkCountHistogramName, 2, 1);
  histogram_tester_.ExpectTotalCount(kBookmarkCountHistogramName, 3);
}

TEST_F(PageMetricsUnitTest, FirstPageLoadTimeImmediate) {
  task_environment_.FastForwardBy(base::Minutes(1));
  histogram_tester_.ExpectTotalCount(kFirstPageLoadTimeHistogramName, 0);

  page_metrics_service_->IncrementPagesLoadedCount(false, false);
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Hours(2));

  page_metrics_service_->IncrementPagesLoadedCount(false, false);
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 1, 1);
}

TEST_F(PageMetricsUnitTest, FirstPageLoadTimeLater) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  task_environment_.FastForwardBy(base::Minutes(30));
  histogram_tester_.ExpectTotalCount(kFirstPageLoadTimeHistogramName, 0);

  page_metrics_service_->IncrementPagesLoadedCount(false, false);
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Days(2));

  page_metrics_service_->IncrementPagesLoadedCount(false, false);
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Days(8));
  page_metrics_service_->IncrementPagesLoadedCount(false, false);
  histogram_tester_.ExpectUniqueSample(kFirstPageLoadTimeHistogramName, 2, 1);
}

TEST_F(PageMetricsUnitTest, FirstPageLoadTimeTooLate) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kFirstPageLoadTimeHistogramName, 0);

  page_metrics_service_->IncrementPagesLoadedCount(false, false);
  histogram_tester_.ExpectTotalCount(kFirstPageLoadTimeHistogramName, 0);
}

TEST_F(PageMetricsUnitTest, CombinedSearchStudyNotReportedTooEarly) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  page_metrics_service_->RecordOmniboxQuery();

  task_environment_.FastForwardBy(base::Hours(23));
  histogram_tester_.ExpectTotalCount(kCombinedSearchStudyHistogramName, 0);
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudySearchQueryAttributeName),
            std::nullopt);
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudyMediaUsageAttributeName),
            std::nullopt);
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudyUniquePagesAttributeName),
            std::nullopt);

  task_environment_.FastForwardBy(base::Hours(2));
  histogram_tester_.ExpectUniqueSample(kCombinedSearchStudyHistogramName, false,
                                       1);
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudySearchQueryAttributeName),
            "true");
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudyMediaUsageAttributeName),
            "0");
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudyUniquePagesAttributeName),
            "0");
}

TEST_F(PageMetricsUnitTest, CombinedSearchStudyAttributes) {
#if BUILDFLAG(IS_MAC)
  // TODO(crbug.com/434660312): Re-enable on macOS 26 once issues with
  // unexpected test timeout failures are resolved.
  if (base::mac::MacOSMajorVersion() >= 26) {
    GTEST_SKIP() << "Disabled on macOS Tahoe and later.";
  }
#endif
  history_service_->AddPage(GURL("https://example.com"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);
  media_session_metrics_.NotifyMediaPlaybackTick(base::Minutes(10));
  page_metrics_service_->RecordOmniboxQuery();

  task_environment_.FastForwardBy(base::Days(1) + base::Minutes(1));

  histogram_tester_.ExpectUniqueSample(kCombinedSearchStudyHistogramName, false,
                                       1);
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudySearchQueryAttributeName),
            "true");
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudyMediaUsageAttributeName),
            "6-30");
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudyUniquePagesAttributeName),
            "1-4");

  // Frame resets after report; nothing new happened in the following period.
  task_environment_.FastForwardBy(base::Days(1) + base::Minutes(1));
  histogram_tester_.ExpectUniqueSample(kCombinedSearchStudyHistogramName, false,
                                       2);
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudySearchQueryAttributeName),
            "false");
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudyMediaUsageAttributeName),
            "0");
  EXPECT_EQ(relay_observer_.GetCustomAttribute(
                kCombinedSearchStudyUniquePagesAttributeName),
            "0");
}

}  // namespace misc_metrics
