/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/guid.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_unittest_util.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/serving/ad_notifications/ad_notification_serving.h"
#include "bat/ads/internal/serving/permission_rules/user_activity_permission_rule_unittest_util.h"
#include "bat/ads/internal/serving/targeting/geographic/subdivision/subdivision_targeting.h"
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
  targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::Serving serving(&subdivision_targeting,
                                    &anti_targeting_resource);

  serving.MaybeServeAd();
}

}  // namespace

class BatAdsPriorityTest : public UnitTestBase {
 protected:
  BatAdsPriorityTest()
      : database_table_(
            std::make_unique<database::table::CreativeAdNotifications>()) {}

  ~BatAdsPriorityTest() override = default;

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

    ForceUserActivityFrequencyCapPermission();
  }

  CreativeAdNotificationInfo BuildCreativeAdNotification() {
    CreativeAdNotificationInfo creative_ad;

    creative_ad.creative_instance_id =
        base::GUID::GenerateRandomV4().AsLowercaseString();
    creative_ad.creative_set_id =
        base::GUID::GenerateRandomV4().AsLowercaseString();
    creative_ad.campaign_id =
        base::GUID::GenerateRandomV4().AsLowercaseString();
    creative_ad.start_at = DistantPast();
    creative_ad.end_at = DistantFuture();
    creative_ad.daily_cap = 1;
    creative_ad.advertiser_id =
        base::GUID::GenerateRandomV4().AsLowercaseString();
    creative_ad.priority = 1;
    creative_ad.ptr = 1.0;
    creative_ad.per_day = 1;
    creative_ad.per_week = 1;
    creative_ad.per_month = 1;
    creative_ad.total_max = 1;
    creative_ad.value = 1.0;
    creative_ad.segment = "untargeted";
    creative_ad.geo_targets = {"US"};
    creative_ad.target_url = GURL("https://brave.com");
    CreativeDaypartInfo daypart;
    creative_ad.dayparts = {daypart};
    creative_ad.title = "Test Ad Title";
    creative_ad.body = "Test Ad Body";

    return creative_ad;
  }

  void ServeAdForIterations(const int iterations) {
    for (int i = 0; i < iterations; i++) {
      ResetEligibleAds(AdType::kAdNotification);

      targeting::geographic::SubdivisionTargeting subdivision_targeting;
      resource::AntiTargeting anti_targeting_resource;
      ad_notifications::Serving serving(&subdivision_targeting,
                                        &anti_targeting_resource);

      serving.MaybeServeAd();
    }
  }

  void Save(const CreativeAdNotificationList& creative_ads) {
    database_table_->Save(creative_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeAdNotifications> database_table_;
};

TEST_F(BatAdsPriorityTest, PrioritizeDeliveryForSingleAd) {
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

TEST_F(BatAdsPriorityTest, PrioritizeDeliveryForNoAds) {
  // Arrange

  // Act
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  ServeAd();

  // Assert
}

TEST_F(BatAdsPriorityTest, PrioritizeDeliveryForMultipleAds) {
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
