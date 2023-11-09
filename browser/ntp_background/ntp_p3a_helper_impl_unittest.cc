// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_registry.h"
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
constexpr char kHistogramPrefix[] = "creativeInstanceId.";
constexpr char kCreativeTotalHistogramName[] = "creativeInstanceId.total.count";

constexpr char kClicksEventType[] = "clicks";
constexpr char kViewsEventType[] = "views";
constexpr char kLandsEventType[] = "lands";

constexpr char kTestP3AJsonHost[] = "https://p3a-json.brave.com";
constexpr char kTestP2AJsonHost[] = "https://p2a-json.brave.com";
constexpr char kTestP3ACreativeHost[] = "https://p3a-creative.brave.com";
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
    NTPP3AHelperImpl::RegisterLocalStatePrefs(local_state_.registry());

    brave_ads::RegisterProfilePrefs(prefs_.registry());
    brave_rewards::RegisterProfilePrefs(prefs_.registry());

    p3a::P3AConfig config;
    config.p3a_json_upload_url = GURL(kTestP3AJsonHost);
    config.p2a_json_upload_url = GURL(kTestP2AJsonHost);
    config.p3a_creative_upload_url = GURL(kTestP3ACreativeHost);
    p3a_service_ = scoped_refptr(new p3a::P3AService(
        local_state_, "release", "2049-01-01", std::move(config)));

    ntp_p3a_helper_ = std::make_unique<NTPP3AHelperImpl>(
        &local_state_, p3a_service_.get(), &prefs_);
  }

  std::string GetExpectedHistogramName(const std::string& event_type) {
    return base::StrCat(
        {kHistogramPrefix, kTestCreativeMetricId, ".", event_type});
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;

  scoped_refptr<p3a::P3AService> p3a_service_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable prefs_;

  std::unique_ptr<NTPP3AHelperImpl> ntp_p3a_helper_;
};

TEST_F(NTPP3AHelperImplTest, OneEventTypeCountReported) {
  ntp_p3a_helper_->RecordView(kTestCreativeMetricId);

  const std::string histogram_name = GetExpectedHistogramName(kViewsEventType);

  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());

  histogram_tester_->ExpectTotalCount(histogram_name, 0);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 0);

  // Mock a P3A rotation to trigger just-in-time collection of metrics
  ntp_p3a_helper_->OnP3ARotation(p3a::MetricLogType::kExpress,
                                 /*is_star*/ false);

  histogram_tester_->ExpectUniqueSample(histogram_name, 1, 1);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 1);

  ntp_p3a_helper_->RecordView(kTestCreativeMetricId);
  ntp_p3a_helper_->OnP3ARotation(p3a::MetricLogType::kExpress,
                                 /*is_star*/ false);

  histogram_tester_->ExpectBucketCount(histogram_name, 2, 1);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 2);
  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
  EXPECT_TRUE(p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
                  .has_value());

  ntp_p3a_helper_->OnP3AMetricCycled(histogram_name, /*is_star*/ false);
  ntp_p3a_helper_->OnP3AMetricCycled(kCreativeTotalHistogramName,
                                     /*is_star*/ false);

  histogram_tester_->ExpectTotalCount(histogram_name, 2);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 2);
  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
          .has_value());
}

TEST_F(NTPP3AHelperImplTest, OneEventTypeCountReportedWhileInflight) {
  ntp_p3a_helper_->RecordClickAndMaybeLand(kTestCreativeMetricId);
  ntp_p3a_helper_->RecordClickAndMaybeLand(kTestCreativeMetricId);

  const std::string histogram_name = GetExpectedHistogramName(kClicksEventType);

  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());

  histogram_tester_->ExpectTotalCount(histogram_name, 0);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 0);

  ntp_p3a_helper_->OnP3ARotation(p3a::MetricLogType::kExpress,
                                 /*is_star*/ false);

  histogram_tester_->ExpectBucketCount(histogram_name, 2, 1);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 1);

  // Recorded a click while recorded count is "in-flight"
  ntp_p3a_helper_->RecordClickAndMaybeLand(kTestCreativeMetricId);
  ntp_p3a_helper_->OnP3AMetricCycled(histogram_name, /*is_star*/ false);

  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName));

  ntp_p3a_helper_->OnP3ARotation(p3a::MetricLogType::kExpress,
                                 /*is_star*/ false);
  histogram_tester_->ExpectBucketCount(histogram_name, 1, 1);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 2);

  ntp_p3a_helper_->OnP3AMetricCycled(histogram_name, /*is_star*/ false);
  ntp_p3a_helper_->OnP3AMetricCycled(kCreativeTotalHistogramName,
                                     /*is_star*/ false);

  histogram_tester_->ExpectTotalCount(histogram_name, 2);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 2);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
          .has_value());
}

TEST_F(NTPP3AHelperImplTest, LandCountReported) {
  ntp_p3a_helper_->RecordClickAndMaybeLand(kTestCreativeMetricId);

  const std::string histogram_name = GetExpectedHistogramName(kLandsEventType);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
          .has_value());

  ntp_p3a_helper_->SetLastTabURL(GURL("https://adexample.com/page1"));

  task_environment_.FastForwardBy(base::Seconds(6));

  // It's acceptable to access other pages,
  // as long as they're on the same host.
  ntp_p3a_helper_->SetLastTabURL(GURL("https://adexample.com/page2"));

  task_environment_.FastForwardBy(base::Seconds(5));

  histogram_tester_->ExpectTotalCount(histogram_name, 0);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 0);

  ntp_p3a_helper_->OnP3ARotation(p3a::MetricLogType::kExpress,
                                 /*is_star*/ false);
  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
  EXPECT_TRUE(p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
                  .has_value());
  histogram_tester_->ExpectUniqueSample(histogram_name, 1, 1);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 1);

  ntp_p3a_helper_->RecordClickAndMaybeLand(kTestCreativeMetricId);

  ntp_p3a_helper_->SetLastTabURL(GURL("https://adexample.com/page1"));

  task_environment_.FastForwardBy(base::Seconds(6));
  histogram_tester_->ExpectTotalCount(histogram_name, 1);

  // Should not trigger land, since user left page before "land time"
  ntp_p3a_helper_->SetLastTabURL(GURL("https://differenthost.com/page1"));

  task_environment_.FastForwardBy(base::Seconds(5));

  ntp_p3a_helper_->OnP3ARotation(p3a::MetricLogType::kExpress,
                                 /*is_star*/ false);
  histogram_tester_->ExpectBucketCount(histogram_name, 1, 2);
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 2);

  EXPECT_TRUE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
  EXPECT_TRUE(p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
                  .has_value());

  ntp_p3a_helper_->OnP3AMetricCycled(histogram_name, /*is_star*/ false);
  ntp_p3a_helper_->OnP3AMetricCycled(kCreativeTotalHistogramName,
                                     /*is_star*/ false);

  histogram_tester_->ExpectBucketCount(histogram_name, 1, 2);
  histogram_tester_->ExpectTotalCount(kCreativeTotalHistogramName, 2);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
          .has_value());
}

TEST_F(NTPP3AHelperImplTest, StopSendingAfterEnablingRewards) {
  const std::string histogram_name = GetExpectedHistogramName(kViewsEventType);

  ntp_p3a_helper_->RecordView(kTestCreativeMetricId);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
          .has_value());

  ntp_p3a_helper_->OnP3ARotation(p3a::MetricLogType::kExpress,
                                 /*is_star*/ false);

  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 1);
  EXPECT_TRUE(p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
                  .has_value());

  ntp_p3a_helper_->OnP3AMetricCycled(histogram_name, /*is_star*/ false);
  ntp_p3a_helper_->OnP3AMetricCycled(kCreativeTotalHistogramName,
                                     /*is_star*/ false);

  ntp_p3a_helper_->RecordView(kTestCreativeMetricId);

  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);

  ntp_p3a_helper_->OnP3ARotation(p3a::MetricLogType::kExpress,
                                 /*is_star*/ false);

  // should send total for any outstanding events
  // (such as the event before the second rotation above)
  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 2);
  EXPECT_TRUE(p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
                  .has_value());

  ntp_p3a_helper_->OnP3AMetricCycled(histogram_name, /*is_star*/ false);
  ntp_p3a_helper_->OnP3AMetricCycled(kCreativeTotalHistogramName,
                                     /*is_star*/ false);

  ntp_p3a_helper_->OnP3ARotation(p3a::MetricLogType::kExpress,
                                 /*is_star*/ false);

  histogram_tester_->ExpectUniqueSample(kCreativeTotalHistogramName, 1, 2);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
          .has_value());
  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());

  ntp_p3a_helper_->OnP3AMetricCycled(histogram_name, /*is_star*/ false);
  ntp_p3a_helper_->OnP3AMetricCycled(kCreativeTotalHistogramName,
                                     /*is_star*/ false);

  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(histogram_name).has_value());
  EXPECT_FALSE(
      p3a_service_->GetDynamicMetricLogType(kCreativeTotalHistogramName)
          .has_value());
}

}  // namespace ntp_background_images
