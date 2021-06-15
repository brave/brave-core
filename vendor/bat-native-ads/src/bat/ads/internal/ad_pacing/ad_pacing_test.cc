/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
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

}  // namespace

class BatAdsAdPacingTest : public UnitTestBase {
 protected:
  BatAdsAdPacingTest()
      : database_table_(
            std::make_unique<database::table::CreativeAdNotifications>()) {}

  ~BatAdsAdPacingTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* integration_test */ true);

    MockLoad(ads_client_mock_, "confirmations.json",
             "confirmations_with_unblinded_tokens.json");

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

  CreativeAdNotificationInfo GetCreativeAdNotification1() {
    CreativeAdNotificationInfo creative_ad_notification;

    creative_ad_notification.creative_instance_id =
        "3519f52c-46a4-4c48-9c2b-c264c0067f04";
    creative_ad_notification.creative_set_id =
        "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
    creative_ad_notification.campaign_id =
        "84197fc8-830a-4a8e-8339-7a70c2bfa104";
    creative_ad_notification.start_at_timestamp = DistantPastAsTimestamp();
    creative_ad_notification.end_at_timestamp = DistantFutureAsTimestamp();
    creative_ad_notification.daily_cap = 1;
    creative_ad_notification.advertiser_id =
        "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
    creative_ad_notification.priority = 1;
    creative_ad_notification.ptr = 1.0;
    creative_ad_notification.per_day = 3;
    creative_ad_notification.per_week = 4;
    creative_ad_notification.per_month = 5;
    creative_ad_notification.total_max = 6;
    creative_ad_notification.segment = "untargeted";
    creative_ad_notification.geo_targets = {"US"};
    creative_ad_notification.target_url = "https://brave.com/1";
    CreativeDaypartInfo daypart;
    creative_ad_notification.dayparts = {daypart};
    creative_ad_notification.title = "Test Ad 1 Title";
    creative_ad_notification.body = "Test Ad 1 Body";

    return creative_ad_notification;
  }

  CreativeAdNotificationInfo GetCreativeAdNotification2() {
    CreativeAdNotificationInfo creative_ad_notification;

    creative_ad_notification.creative_instance_id =
        "a1ac44c2-675f-43e6-ab6d-500614cafe63";
    creative_ad_notification.creative_set_id =
        "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
    creative_ad_notification.campaign_id =
        "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
    creative_ad_notification.start_at_timestamp = DistantPastAsTimestamp();
    creative_ad_notification.end_at_timestamp = DistantFutureAsTimestamp();
    creative_ad_notification.daily_cap = 1;
    creative_ad_notification.advertiser_id =
        "9a11b60f-e29d-4446-8d1f-318311e36e0a";
    creative_ad_notification.priority = 2;
    creative_ad_notification.ptr = 1.0;
    creative_ad_notification.per_day = 3;
    creative_ad_notification.per_week = 4;
    creative_ad_notification.per_month = 5;
    creative_ad_notification.total_max = 6;
    creative_ad_notification.segment = "untargeted";
    creative_ad_notification.geo_targets = {"US"};
    creative_ad_notification.target_url = "https://brave.com/2";
    CreativeDaypartInfo daypart;
    creative_ad_notification.dayparts = {daypart};
    creative_ad_notification.title = "Test Ad 2 Title";
    creative_ad_notification.body = "Test Ad 2 Body";

    return creative_ad_notification;
  }

  void ServeAdForIterations(const int iterations) {
    for (int i = 0; i < iterations; i++) {
      ResetFrequencyCaps(AdType::kAdNotification);

      AdTargeting ad_targeting;
      ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
      resource::AntiTargeting anti_targeting_resource;
      ad_notifications::AdServing ad_serving(
          &ad_targeting, &subdivision_targeting, &anti_targeting_resource);

      ad_serving.MaybeServeAd();
    }
  }

  void Save(const CreativeAdNotificationList& creative_ad_notifications) {
    database_table_->Save(creative_ad_notifications, [](const Result result) {
      ASSERT_EQ(Result::SUCCESS, result);
    });
  }

  std::unique_ptr<database::table::CreativeAdNotifications> database_table_;
};

TEST_F(BatAdsAdPacingTest, PacingDisableDelivery) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotification1();
  creative_ad_notification_1.ptr = 0.0;
  creative_ad_notifications.push_back(creative_ad_notification_1);

  Save(creative_ad_notifications);

  // Act
  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  ServeAdForIterations(iterations);

  // Assert
}

TEST_F(BatAdsAdPacingTest, NoPacing) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotification1();
  creative_ad_notification_1.ptr = 1.0;
  creative_ad_notifications.push_back(creative_ad_notification_1);

  Save(creative_ad_notifications);

  // Act
  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(iterations);

  ServeAdForIterations(iterations);

  // Assert
}

TEST_F(BatAdsAdPacingTest, SimplePacing) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotification1();
  creative_ad_notification_1.ptr = 0.2;
  creative_ad_notifications.push_back(creative_ad_notification_1);

  Save(creative_ad_notifications);

  // Act
  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_))
      .Times(Between(iterations * creative_ad_notification_1.ptr * 0.8,
                     iterations * creative_ad_notification_1.ptr * 1.2));

  ServeAdForIterations(iterations);

  // Assert
}

TEST_F(BatAdsAdPacingTest, NoPacingPrioritized) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  const CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotification1();
  creative_ad_notifications.push_back(creative_ad_notification_1);

  const CreativeAdNotificationInfo creative_ad_notification_2 =
      GetCreativeAdNotification2();
  creative_ad_notifications.push_back(creative_ad_notification_2);

  Save(creative_ad_notifications);

  // Act
  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(DoesMatchCreativeInstanceId(
                  creative_ad_notification_1.creative_instance_id)))
      .Times(1);

  ServeAdForIterations(1);

  // Assert
}

TEST_F(BatAdsAdPacingTest, PacingDisableDeliveryPrioritized) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotification1();
  creative_ad_notification_1.ptr = 0.0;
  creative_ad_notifications.push_back(creative_ad_notification_1);

  const CreativeAdNotificationInfo creative_ad_notification_2 =
      GetCreativeAdNotification2();
  creative_ad_notifications.push_back(creative_ad_notification_2);

  Save(creative_ad_notifications);

  // Act
  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(DoesMatchCreativeInstanceId(
                  creative_ad_notification_2.creative_instance_id)))
      .Times(1);

  ServeAdForIterations(1);

  // Assert
}

TEST_F(BatAdsAdPacingTest, PacingAndPrioritization) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotification1();
  creative_ad_notification_1.ptr = 0.5;
  creative_ad_notifications.push_back(creative_ad_notification_1);

  CreativeAdNotificationInfo creative_ad_notification_2 =
      GetCreativeAdNotification2();
  creative_ad_notification_2.ptr = 0.5;
  creative_ad_notifications.push_back(creative_ad_notification_2);

  Save(creative_ad_notifications);

  // Act
  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(DoesMatchCreativeInstanceId(
                  creative_ad_notification_1.creative_instance_id)))
      .Times(Between(iterations * creative_ad_notification_1.ptr * 0.8,
                     iterations * creative_ad_notification_1.ptr * 1.2));

  // creative_ad_notification_2 ad would be shown probabilistically when
  // creative_ad_notification_1 gets dropped due to pacing
  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(DoesMatchCreativeInstanceId(
                  creative_ad_notification_2.creative_instance_id)))
      .Times(Between(iterations * creative_ad_notification_1.ptr * 0.8 *
                         creative_ad_notification_2.ptr,
                     iterations * creative_ad_notification_1.ptr * 1.2 *
                         creative_ad_notification_2.ptr));

  ServeAdForIterations(iterations);

  // Assert
}

}  // namespace ads
