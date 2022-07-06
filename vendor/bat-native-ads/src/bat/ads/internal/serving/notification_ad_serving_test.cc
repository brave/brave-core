/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/notification_ad_serving.h"

#include <map>

#include "base/guid.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/base/net/http/http_status_code.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/serving/permission_rules/user_activity_permission_rule_unittest_util.h"
#include "bat/ads/internal/serving/serving_features.h"
#include "bat/ads/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;
using ::testing::AllOf;
using ::testing::Between;
using ::testing::Field;
using ::testing::Matcher;

namespace ads {
namespace notification_ads {

namespace {

Matcher<const NotificationAdInfo&> DoesMatchCreativeInstanceId(
    const std::string& creative_instance_id) {
  return AllOf(Field("creative_instance_id",
                     &NotificationAdInfo::creative_instance_id,
                     creative_instance_id));
}

}  // namespace

class BatAdsNotificationAdServingIntegrationTest : public UnitTestBase {
 protected:
  BatAdsNotificationAdServingIntegrationTest() = default;

  ~BatAdsNotificationAdServingIntegrationTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* is_integration_test */ true);
  }

  void SetUpMocks() override {
    CopyFileFromTestPathToTempPath("confirmations_with_unblinded_tokens.json",
                                   kConfirmationsFilename);

    const URLEndpointMap endpoints = {
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
  }

  void ServeAd() {
    geographic::SubdivisionTargeting subdivision_targeting;
    resource::AntiTargeting anti_targeting_resource;
    Serving serving(&subdivision_targeting, &anti_targeting_resource);

    serving.MaybeServeAd();
  }

  void Save(const CreativeNotificationAdList& creative_ads) {
    database::table::CreativeNotificationAds database_table;
    database_table.Save(creative_ads,
                        [](const bool success) { ASSERT_TRUE(success); });
  }
};

TEST_F(BatAdsNotificationAdServingIntegrationTest, ServeAd) {
  // Arrange
  ForceUserActivityPermissionRule();

  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                     creative_ad.creative_instance_id)))
      .Times(1);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsNotificationAdServingIntegrationTest,
       DoNotServeAdIfNoEligibleAdsFound) {
  // Arrange
  ForceUserActivityPermissionRule();

  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsNotificationAdServingIntegrationTest, DoNotServeInvalidAd) {
  // Arrange
  ForceUserActivityPermissionRule();

  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsNotificationAdServingIntegrationTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  // Act
  ServeAd();

  // Assert
}

TEST_F(BatAdsNotificationAdServingIntegrationTest, ServeAdWithServingVersion2) {
  // Arrange
  ForceUserActivityPermissionRule();

  CreativeNotificationAdList creative_ads;
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  Serving serving(&subdivision_targeting, &anti_targeting_resource);

  std::map<std::string, std::string> serving_parameters;
  serving_parameters["ad_serving_version"] = "2";

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{features::kServing, serving_parameters}}, {});

  ASSERT_TRUE(features::IsServingEnabled());
  ASSERT_EQ(2, features::GetServingVersion());

  // Act
  EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                     creative_ad.creative_instance_id)))
      .Times(1);

  ServeAd();

  // Assert
}

}  // namespace notification_ads
}  // namespace ads
