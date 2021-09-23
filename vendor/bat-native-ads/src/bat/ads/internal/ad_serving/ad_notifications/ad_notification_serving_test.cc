/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"

#include <map>
#include <string>

#include "base/guid.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
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
    ASSERT_TRUE(CopyFileFromTestPathToTempDir(
        "confirmations_with_unblinded_tokens.json", "confirmations.json"));

    UnitTestBase::SetUpForTesting(/* integration_test */ true);

    const URLEndpoints endpoints = {
        {"/v8/catalog", {{net::HTTP_OK, "/empty_catalog.json"}}}};
    MockUrlRequest(ads_client_mock_, endpoints);

    InitializeAds();
  }

  void RecordUserActivityEvents() {
    UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
    UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  }

  void ServeAd() {
    ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
    resource::AntiTargeting anti_targeting_resource;
    ad_notifications::AdServing ad_serving(&subdivision_targeting,
                                           &anti_targeting_resource);

    ad_serving.MaybeServeAd();
  }

  void Save(const CreativeAdNotificationList& creative_ad_notifications) {
    database_table_->Save(creative_ad_notifications,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeAdNotifications> database_table_;
};

TEST_F(BatAdsAdNotificationServingTest, ServeAd) {
  // Arrange
  RecordUserActivityEvents();

  CreativeAdNotificationList creative_ad_notifications;
  CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotification();
  creative_ad_notifications.push_back(creative_ad_notification);
  Save(creative_ad_notifications);

  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(DoesMatchCreativeInstanceId(
                  creative_ad_notification.creative_instance_id)))
      .Times(1);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsAdNotificationServingTest, DoNotServeAdIfNoEligibleAdsFound) {
  // Arrange
  RecordUserActivityEvents();

  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsAdNotificationServingTest, DoNotServeInvalidAd) {
  // Arrange
  RecordUserActivityEvents();

  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsAdNotificationServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;
  CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotification();
  creative_ad_notifications.push_back(creative_ad_notification);
  Save(creative_ad_notifications);

  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsAdNotificationServingTest, ServeAdWithAdServingVersion2) {
  // Arrange
  RecordUserActivityEvents();

  CreativeAdNotificationList creative_ad_notifications;
  const CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotification();
  creative_ad_notifications.push_back(creative_ad_notification);
  Save(creative_ad_notifications);

  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::AdServing ad_serving(&subdivision_targeting,
                                         &anti_targeting_resource);

  std::map<std::string, std::string> ad_serving_parameters;
  ad_serving_parameters["ad_serving_version"] = 2;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{features::kAdServing, ad_serving_parameters}}, {});

  // Act
  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(DoesMatchCreativeInstanceId(
                  creative_ad_notification.creative_instance_id)))
      .Times(1);

  ServeAd();

  // Assert
}

}  // namespace ads
