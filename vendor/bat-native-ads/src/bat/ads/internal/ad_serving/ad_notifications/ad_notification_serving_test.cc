/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"

#include <string>

#include "base/guid.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
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

class BatAdsAdNotificationServingTest : public UnitTestBase {
 protected:
  BatAdsAdNotificationServingTest()
      : database_table_(
            std::make_unique<database::table::CreativeAdNotifications>()) {}

  ~BatAdsAdNotificationServingTest() override = default;

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

  CreativeAdNotificationInfo GetCreativeAdNotification() {
    CreativeAdNotificationInfo creative_ad_notification;

    creative_ad_notification.creative_instance_id = base::GenerateGUID();
    creative_ad_notification.creative_set_id = base::GenerateGUID();
    creative_ad_notification.campaign_id = base::GenerateGUID();
    creative_ad_notification.start_at_timestamp = DistantPastAsTimestamp();
    creative_ad_notification.end_at_timestamp = DistantFutureAsTimestamp();
    creative_ad_notification.daily_cap = 1;
    creative_ad_notification.advertiser_id = base::GenerateGUID();
    creative_ad_notification.priority = 1;
    creative_ad_notification.ptr = 1.0;
    creative_ad_notification.per_day = 1;
    creative_ad_notification.per_week = 1;
    creative_ad_notification.per_month = 1;
    creative_ad_notification.total_max = 1;
    creative_ad_notification.segment = "untargeted";
    creative_ad_notification.geo_targets = {"US"};
    creative_ad_notification.target_url = "https://brave.com";
    CreativeDaypartInfo daypart;
    creative_ad_notification.dayparts = {daypart};
    creative_ad_notification.title = "Test Ad Title";
    creative_ad_notification.body = "Test Ad Body";

    return creative_ad_notification;
  }

  void ServeAd() {
    AdTargeting ad_targeting;
    ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
    resource::AntiTargeting anti_targeting_resource;
    ad_notifications::AdServing ad_serving(
        &ad_targeting, &subdivision_targeting, &anti_targeting_resource);

    ad_serving.MaybeServeAd();
  }

  void Save(const CreativeAdNotificationList& creative_ad_notifications) {
    database_table_->Save(creative_ad_notifications, [](const Result result) {
      ASSERT_EQ(Result::SUCCESS, result);
    });
  }

  std::unique_ptr<database::table::CreativeAdNotifications> database_table_;
};

TEST_F(BatAdsAdNotificationServingTest, ServeAd) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotification();
  creative_ad_notifications.push_back(creative_ad_notification);

  Save(creative_ad_notifications);

  // Act
  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(DoesMatchCreativeInstanceId(
                  creative_ad_notification.creative_instance_id)))
      .Times(1);

  ServeAd();

  // Assert
}

}  // namespace ads
