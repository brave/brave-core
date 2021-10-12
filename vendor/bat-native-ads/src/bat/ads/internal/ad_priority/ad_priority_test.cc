/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/guid.h"
#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/internal/user_activity/user_activity.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;
using ::testing::AllOf;
using ::testing::Between;
using ::testing::Field;
using ::testing::Matcher;

namespace ads {

namespace {

Matcher<const AdNotificationInfo&> DoesMatchCreativeInstanceId(
    const std::string& creative_instance_id) {
  return AllOf(Field("creative_instance_id",
                     &AdNotificationInfo::creative_instance_id,
                     creative_instance_id));
}

void ServeAd() {
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::AdServing ad_serving(&subdivision_targeting,
                                         &anti_targeting_resource);

  ad_serving.MaybeServeAd();
}

}  // namespace

class BatAdsAdPriorityTest : public UnitTestBase {
 protected:
  BatAdsAdPriorityTest()
      : database_table_(
            std::make_unique<database::table::CreativeAdNotifications>()) {}

  ~BatAdsAdPriorityTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(CopyFileFromTestPathToTempDir(
        "confirmations_with_unblinded_tokens.json", "confirmations.json"));

    UnitTestBase::SetUpForTesting(/* integration_test */ true);

    const URLEndpoints endpoints = {
        {"/v8/catalog", {{net::HTTP_OK, "/empty_catalog.json"}}}};
    MockUrlRequest(ads_client_mock_, endpoints);

    InitializeAds();

    RecordUserActivityEvents();
  }

  void RecordUserActivityEvents() {
    UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
    UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  }

  CreativeAdNotificationInfo BuildCreativeAdNotification() {
    CreativeAdNotificationInfo creative_ad;

    creative_ad.creative_instance_id = base::GenerateGUID();
    creative_ad.creative_set_id = base::GenerateGUID();
    creative_ad.campaign_id = base::GenerateGUID();
    creative_ad.start_at = DistantPast();
    creative_ad.end_at = DistantFuture();
    creative_ad.daily_cap = 1;
    creative_ad.advertiser_id = base::GenerateGUID();
    creative_ad.priority = 1;
    creative_ad.ptr = 1.0;
    creative_ad.per_day = 1;
    creative_ad.per_week = 1;
    creative_ad.per_month = 1;
    creative_ad.total_max = 1;
    creative_ad.value = 1.0;
    creative_ad.segment = "untargeted";
    creative_ad.geo_targets = {"US"};
    creative_ad.target_url = "https://brave.com";
    CreativeDaypartInfo daypart;
    creative_ad.dayparts = {daypart};
    creative_ad.title = "Test Ad Title";
    creative_ad.body = "Test Ad Body";

    return creative_ad;
  }

  void ServeAdForIterations(const int iterations) {
    for (int i = 0; i < iterations; i++) {
      ResetFrequencyCaps(AdType::kAdNotification);

      ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
      resource::AntiTargeting anti_targeting_resource;
      ad_notifications::AdServing ad_serving(&subdivision_targeting,
                                             &anti_targeting_resource);

      ad_serving.MaybeServeAd();
    }
  }

  void Save(const CreativeAdNotificationList& creative_ads) {
    database_table_->Save(creative_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeAdNotifications> database_table_;
};

TEST_F(BatAdsAdPriorityTest, PrioritizeDeliveryForSingleAd) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  CreativeAdNotificationInfo creative_ad = BuildCreativeAdNotification();
  creative_ad.priority = 3;
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                     creative_ad.creative_instance_id)))
      .Times(1);

  ServeAd();

  // Assert
}

TEST_F(BatAdsAdPriorityTest, PrioritizeDeliveryForNoAds) {
  // Arrange

  // Act
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  ServeAd();

  // Assert
}

TEST_F(BatAdsAdPriorityTest, PrioritizeDeliveryForMultipleAds) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  CreativeAdNotificationInfo creative_ad_1 = BuildCreativeAdNotification();
  creative_ad_1.priority = 3;
  creative_ads.push_back(creative_ad_1);

  CreativeAdNotificationInfo creative_ad_2 = BuildCreativeAdNotification();
  creative_ad_2.priority = 2;
  creative_ads.push_back(creative_ad_2);

  CreativeAdNotificationInfo creative_ad_3 = BuildCreativeAdNotification();
  creative_ad_3.priority = 4;
  creative_ads.push_back(creative_ad_3);

  Save(creative_ads);

  // Act
  EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                     creative_ad_2.creative_instance_id)))
      .Times(1);

  ServeAd();

  // Assert
}

}  // namespace ads
