// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_rewards/core/pref_registry.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/p3a_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

constexpr char kTestCreativeMetricId[] = "2ba3659a-4737-4c4e-892a-a6a2e0e2a871";
constexpr char kTestCampaign1[] = "40a357fd-a6e3-485c-92a0-7ff057dd7686";
constexpr char kTestCampaign2[] = "a5d13b23-a59d-4a3f-a92b-499edd5dfce4";
constexpr char kCreativeHistogramPrefix[] = "creativeInstanceId.";
constexpr char kCampaignHistogramPrefix[] = "campaignId.";
constexpr char kCreativeTotalHistogramName[] = "creativeInstanceId.total.count";

constexpr char kClicksEventType[] = "clicks";
constexpr char kViewsEventType[] = "views";
constexpr char kLandsEventType[] = "lands";
constexpr char kAwareEventType[] = "aware";
constexpr char kViewedEventType[] = "viewed";

}  // namespace

class NTPP3AHelperImplTest : public testing::Test {
 public:
  NTPP3AHelperImplTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    histogram_tester_ = std::make_unique<base::HistogramTester>();

    brave::RegisterPrefsForBraveReferralsService(local_state_.registry());
    p3a::P3AService::RegisterPrefs(local_state_.registry(),
                                   /*first_run*/ false);
    brave_ads::RegisterLocalStatePrefs(local_state_.registry());
    brave_ads::RegisterLocalStatePrefsForMigration(local_state_.registry());
    brave_ads::RegisterProfilePrefs(prefs_.registry());
    brave_rewards::RegisterProfilePrefs(prefs_.registry());

    p3a::P3AConfig config;
    base::Time install_time;
    ASSERT_TRUE(base::Time::FromString("2049-01-01", &install_time));
    p3a_service_ = scoped_refptr(new p3a::P3AService(
        local_state_, "release", install_time, std::move(config)));

    ntp_p3a_helper_ = std::make_unique<NTPP3AHelperImpl>(
        &local_state_, p3a_service_.get(),
        g_brave_browser_process->ntp_background_images_service(), &prefs_);
  }

  void TearDown() override {
    ntp_p3a_helper_ =
        nullptr;  // It depends on the service to not dangle, so reset it first.
    p3a_service_ = nullptr;
  }

  std::string GetExpectedCreativeHistogramName(const std::string& event_type) {
    return base::StrCat(
        {kCreativeHistogramPrefix, kTestCreativeMetricId, ".", event_type});
  }

  std::string GetExpectedCampaignHistogramName(const std::string& campaign_id,
                                               const std::string& event_type) {
    return base::StrCat(
        {kCampaignHistogramPrefix, campaign_id, ".", event_type});
  }

  void NotifyRotation() {
    ntp_p3a_helper_->OnP3ARotation(p3a::MetricLogType::kExpress);
  }

  void NotifyMetricCycle(const std::string& histogram_name) {
    ntp_p3a_helper_->OnP3AMetricCycled(histogram_name);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;

  scoped_refptr<p3a::P3AService> p3a_service_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable prefs_;

  std::unique_ptr<NTPP3AHelperImpl> ntp_p3a_helper_;
};

TEST_F(NTPP3AHelperImplTest, OneEventTypeCountReported) {
  ntp_p3a_helper_->RecordView(kTestCreativeMetricId, kTestCampaign1);

  const std::string histogram_name =
      GetExpectedCreativeHistogramName(kViewsEventType);

  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());

  histogram_tester_->ExpectTotalCount(histogram_name, 0);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 0);

  // Mock a P3A rotation to trigger just-in-time collection of metrics
  NotifyRotation();

  histogram_tester_->ExpectUniqueSample(histogram_name, 1, 1);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 1);

  ntp_p3a_helper_->RecordView(kTestCreativeMetricId, kTestCampaign1);
  NotifyRotation();

  histogram_tester_->ExpectBucketCount(histogram_name, 2, 1);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 2);
  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());

  NotifyMetricCycle(histogram_name);
  NotifyMetricCycle(kCreativeTotalHistogramName);

  histogram_tester_->ExpectTotalCount(histogram_name, 2);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 2);
  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
}

TEST_F(NTPP3AHelperImplTest, OneEventTypeCountReportedWhileInflight) {
  ntp_p3a_helper_->RecordNewTabPageAdEvent(
      brave_ads::mojom::NewTabPageAdEventType::kClicked, kTestCreativeMetricId);
  ntp_p3a_helper_->RecordNewTabPageAdEvent(
      brave_ads::mojom::NewTabPageAdEventType::kClicked, kTestCreativeMetricId);

  const std::string histogram_name =
      GetExpectedCreativeHistogramName(kClicksEventType);

  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());

  histogram_tester_->ExpectTotalCount(histogram_name, 0);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 0);

  NotifyRotation();

  histogram_tester_->ExpectBucketCount(histogram_name, 2, 1);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 1);

  // Recorded a click while recorded count is "in-flight"
  ntp_p3a_helper_->RecordNewTabPageAdEvent(
      brave_ads::mojom::NewTabPageAdEventType::kClicked, kTestCreativeMetricId);
  NotifyMetricCycle(histogram_name);

  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());

  NotifyRotation();
  histogram_tester_->ExpectBucketCount(histogram_name, 1, 1);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 2);

  NotifyMetricCycle(histogram_name);
  NotifyMetricCycle(kCreativeTotalHistogramName);

  histogram_tester_->ExpectTotalCount(histogram_name, 2);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 2);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
}

TEST_F(NTPP3AHelperImplTest, LandCountReported) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      brave_ads::kSiteVisitFeature, {{"page_land_after", "10s"}});

  ntp_p3a_helper_->RecordNewTabPageAdEvent(
      brave_ads::mojom::NewTabPageAdEventType::kClicked, kTestCreativeMetricId);

  const std::string clicks_histogram_name =
      GetExpectedCreativeHistogramName(kClicksEventType);
  const std::string lands_histogram_name =
      GetExpectedCreativeHistogramName(kLandsEventType);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(lands_histogram_name).has_value());

  ntp_p3a_helper_->OnNavigationDidFinish(GURL("https://adexample.com/page1"));

  task_environment_.FastForwardBy(base::Seconds(6));

  // It's acceptable to access other pages,
  // as long as they're on the same host.
  ntp_p3a_helper_->OnNavigationDidFinish(GURL("https://adexample.com/page2"));

  task_environment_.FastForwardBy(base::Seconds(5));

  histogram_tester_->ExpectTotalCount(clicks_histogram_name, 0);
  histogram_tester_->ExpectTotalCount(lands_histogram_name, 0);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 0);

  NotifyRotation();
  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(clicks_histogram_name).has_value());
  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(lands_histogram_name).has_value());
  histogram_tester_->ExpectUniqueSample(clicks_histogram_name, 1, 1);
  histogram_tester_->ExpectUniqueSample(lands_histogram_name, 1, 1);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 1);

  ntp_p3a_helper_->RecordNewTabPageAdEvent(
      brave_ads::mojom::NewTabPageAdEventType::kClicked, kTestCreativeMetricId);

  ntp_p3a_helper_->OnNavigationDidFinish(GURL("https://adexample.com/page1"));

  task_environment_.FastForwardBy(base::Seconds(6));
  histogram_tester_->ExpectTotalCount(clicks_histogram_name, 1);
  histogram_tester_->ExpectTotalCount(lands_histogram_name, 1);

  // Should not trigger land, since user left page before "land time"
  ntp_p3a_helper_->OnNavigationDidFinish(
      GURL("https://differenthost.com/page1"));

  task_environment_.FastForwardBy(base::Seconds(5));

  NotifyRotation();
  histogram_tester_->ExpectBucketCount(clicks_histogram_name, 2, 1);
  histogram_tester_->ExpectTotalCount(clicks_histogram_name, 2);
  histogram_tester_->ExpectBucketCount(lands_histogram_name, 1, 2);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 2);

  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(clicks_histogram_name).has_value());
  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(lands_histogram_name).has_value());

  NotifyMetricCycle(clicks_histogram_name);
  NotifyMetricCycle(lands_histogram_name);
  NotifyMetricCycle(kCreativeTotalHistogramName);

  histogram_tester_->ExpectTotalCount(clicks_histogram_name, 2);
  histogram_tester_->ExpectBucketCount(lands_histogram_name, 1, 2);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 2);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(clicks_histogram_name).has_value());
  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(lands_histogram_name).has_value());
}

TEST_F(NTPP3AHelperImplTest, StopSendingAfterEnablingRewards) {
  const std::string histogram_name =
      GetExpectedCreativeHistogramName(kViewsEventType);

  ntp_p3a_helper_->RecordView(kTestCreativeMetricId, kTestCampaign1);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
          .has_value());

  NotifyRotation();

  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 1);

  NotifyMetricCycle(histogram_name);
  NotifyMetricCycle(kCreativeTotalHistogramName);

  ntp_p3a_helper_->RecordView(kTestCreativeMetricId, kTestCampaign1);

  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);

  NotifyRotation();

  // should send total for any outstanding events
  // (such as the event before the second rotation above)
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 2);

  NotifyMetricCycle(histogram_name);
  NotifyMetricCycle(kCreativeTotalHistogramName);

  NotifyRotation();

  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 2);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());

  NotifyMetricCycle(histogram_name);
  NotifyMetricCycle(kCreativeTotalHistogramName);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
}

TEST_F(NTPP3AHelperImplTest, CampaignMetricReporting) {
  NTPSponsoredImagesData data;
  data.campaigns.emplace_back().campaign_id = kTestCampaign1;
  data.campaigns.emplace_back().campaign_id = kTestCampaign2;

  auto campaign1_aware_histogram =
      GetExpectedCampaignHistogramName(kTestCampaign1, kAwareEventType);
  auto campaign2_aware_histogram =
      GetExpectedCampaignHistogramName(kTestCampaign2, kAwareEventType);
  auto campaign1_viewed_histogram =
      GetExpectedCampaignHistogramName(kTestCampaign1, kViewedEventType);
  auto campaign2_viewed_histogram =
      GetExpectedCampaignHistogramName(kTestCampaign2, kViewedEventType);

  for (size_t i = 0; i < 3; i++) {
    ntp_p3a_helper_->CheckLoadedCampaigns(data);

    histogram_tester_->ExpectUniqueSample(campaign1_aware_histogram, 1, 1);
    histogram_tester_->ExpectUniqueSample(campaign2_aware_histogram, 1, 1);
    histogram_tester_->ExpectTotalCount(campaign1_viewed_histogram, 0);
    histogram_tester_->ExpectTotalCount(campaign2_viewed_histogram, 0);
    EXPECT_TRUE(p3a_service_->GetDynamicMetricLogType(campaign1_aware_histogram)
                    .has_value());
    EXPECT_TRUE(p3a_service_->GetDynamicMetricLogType(campaign2_aware_histogram)
                    .has_value());
    EXPECT_FALSE(
        p3a_service_->GetDynamicMetricLogType(campaign1_viewed_histogram)
            .has_value());
    EXPECT_FALSE(
        p3a_service_->GetDynamicMetricLogType(campaign2_viewed_histogram)
            .has_value());
  }

  auto expect_status_quo = [&]() {
    histogram_tester_->ExpectUniqueSample(campaign1_aware_histogram, 1, 1);
    histogram_tester_->ExpectUniqueSample(campaign2_aware_histogram, 1, 1);
    histogram_tester_->ExpectUniqueSample(campaign1_viewed_histogram, 1, 1);
    histogram_tester_->ExpectTotalCount(campaign2_viewed_histogram, 0);
    EXPECT_TRUE(p3a_service_->GetDynamicMetricLogType(campaign1_aware_histogram)
                    .has_value());
    EXPECT_TRUE(p3a_service_->GetDynamicMetricLogType(campaign2_aware_histogram)
                    .has_value());
    EXPECT_TRUE(
        p3a_service_->GetDynamicMetricLogType(campaign1_viewed_histogram)
            .has_value());
    EXPECT_FALSE(
        p3a_service_->GetDynamicMetricLogType(campaign2_viewed_histogram)
            .has_value());
  };

  for (size_t i = 0; i < 3; i++) {
    ntp_p3a_helper_->RecordView(kTestCreativeMetricId, kTestCampaign1);
    expect_status_quo();
  }

  task_environment_.FastForwardBy(base::Days(15));
  NotifyRotation();
  expect_status_quo();

  task_environment_.FastForwardBy(base::Days(16));
  NotifyRotation();
  EXPECT_FALSE(p3a_service_->GetDynamicMetricLogType(campaign1_aware_histogram)
                   .has_value());
  EXPECT_FALSE(p3a_service_->GetDynamicMetricLogType(campaign2_aware_histogram)
                   .has_value());
  EXPECT_FALSE(p3a_service_->GetDynamicMetricLogType(campaign1_viewed_histogram)
                   .has_value());
  EXPECT_FALSE(p3a_service_->GetDynamicMetricLogType(campaign2_viewed_histogram)
                   .has_value());
}

}  // namespace ntp_background_images
