/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"

#include <map>
#include <string>

#include "base/guid.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/frequency_capping/permission_rules/user_activity_permission_rule_unittest_util.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
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

    UnitTestBase::SetUpForTesting(/* is_integration_test */ true);

    const URLEndpoints endpoints = {
        {"/v9/catalog", {{net::HTTP_OK, "/empty_catalog.json"}}},
        {// Get issuers request
         R"(/v1/issuers/)",
         {{net::HTTP_OK, R"(
        {
          "ping": 7200000,
          "issuers": [
            {
              "name": "confirmations",
              "publicKeys": [
                {
                  "publicKey": "JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=",
                  "associatedValue": ""
                }
              ]
            },
            {
              "name": "payments",
              "publicKeys": [
                {
                  "publicKey": "JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=",
                  "associatedValue": "0.0"
                },
                {
                  "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                  "associatedValue": "0.1"
                }
              ]
            }
          ]
        }
        )"}}}};
    MockUrlRequest(ads_client_mock_, endpoints);

    InitializeAds();
  }

  void ServeAd() {
    ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
    resource::AntiTargeting anti_targeting_resource;
    ad_notifications::AdServing ad_serving(&subdivision_targeting,
                                           &anti_targeting_resource);

    ad_serving.MaybeServeAd();
  }

  void Save(const CreativeAdNotificationList& creative_ads) {
    database_table_->Save(creative_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeAdNotifications> database_table_;
};

TEST_F(BatAdsAdNotificationServingTest, ServeAd) {
  // Arrange
  ForceUserActivityFrequencyCapPermission();

  CreativeAdNotificationList creative_ads;
  CreativeAdNotificationInfo creative_ad = BuildCreativeAdNotification();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                     creative_ad.creative_instance_id)))
      .Times(1);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsAdNotificationServingTest, DoNotServeAdIfNoEligibleAdsFound) {
  // Arrange
  ForceUserActivityFrequencyCapPermission();

  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsAdNotificationServingTest, DoNotServeInvalidAd) {
  // Arrange
  ForceUserActivityFrequencyCapPermission();

  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsAdNotificationServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  CreativeAdNotificationList creative_ads;
  CreativeAdNotificationInfo creative_ad = BuildCreativeAdNotification();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsAdNotificationServingTest, ServeAdWithAdServingVersion2) {
  // Arrange
  ForceUserActivityFrequencyCapPermission();

  CreativeAdNotificationList creative_ads;
  const CreativeAdNotificationInfo creative_ad = BuildCreativeAdNotification();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::AdServing ad_serving(&subdivision_targeting,
                                         &anti_targeting_resource);

  std::map<std::string, std::string> ad_serving_parameters;
  ad_serving_parameters["ad_serving_version"] = "2";

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{features::kAdServing, ad_serving_parameters}}, {});

  ASSERT_TRUE(features::IsAdServingEnabled());
  ASSERT_EQ(2, features::GetAdServingVersion());

  // Act
  EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                     creative_ad.creative_instance_id)))
      .Times(1);

  ServeAd();

  // Assert
}

}  // namespace ads
