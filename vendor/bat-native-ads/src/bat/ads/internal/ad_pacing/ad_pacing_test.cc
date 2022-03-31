/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_pacing/ad_pacing_random_util.h"
#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/frequency_capping/permission_rules/user_activity_permission_rule_unittest_util.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;
using ::testing::AllOf;
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

std::vector<double> GetAdPacingRandomNumberList() {
  return std::vector<double>{0.0, 0.5, 0.99};
}

}  // namespace

class BatAdsAdPacingTest : public UnitTestBase {
 protected:
  BatAdsAdPacingTest()
      : database_table_(
            std::make_unique<database::table::CreativeAdNotifications>()) {}

  ~BatAdsAdPacingTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(CopyFileFromTestPathToTempDir(
        "confirmations_with_unblinded_tokens.json", "confirmations.json"));

    UnitTestBase::SetUpForTesting(/* integration_test */ true);

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

  CreativeAdNotificationInfo BuildCreativeAdNotification1() {
    CreativeAdNotificationInfo creative_ad;

    creative_ad.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
    creative_ad.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
    creative_ad.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
    creative_ad.start_at = DistantPast();
    creative_ad.end_at = DistantFuture();
    creative_ad.daily_cap = 1;
    creative_ad.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
    creative_ad.priority = 1;
    creative_ad.ptr = 1.0;
    creative_ad.per_day = 3;
    creative_ad.per_week = 4;
    creative_ad.per_month = 5;
    creative_ad.total_max = 6;
    creative_ad.value = 1.0;
    creative_ad.segment = "untargeted";
    creative_ad.geo_targets = {"US"};
    creative_ad.target_url = "https://brave.com/1";
    CreativeDaypartInfo daypart;
    creative_ad.dayparts = {daypart};
    creative_ad.title = "Test Ad 1 Title";
    creative_ad.body = "Test Ad 1 Body";

    return creative_ad;
  }

  CreativeAdNotificationInfo BuildCreativeAdNotification2() {
    CreativeAdNotificationInfo creative_ad;

    creative_ad.creative_instance_id = "a1ac44c2-675f-43e6-ab6d-500614cafe63";
    creative_ad.creative_set_id = "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
    creative_ad.campaign_id = "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
    creative_ad.start_at = DistantPast();
    creative_ad.end_at = DistantFuture();
    creative_ad.daily_cap = 1;
    creative_ad.advertiser_id = "9a11b60f-e29d-4446-8d1f-318311e36e0a";
    creative_ad.priority = 2;
    creative_ad.ptr = 1.0;
    creative_ad.per_day = 3;
    creative_ad.per_week = 4;
    creative_ad.per_month = 5;
    creative_ad.total_max = 6;
    creative_ad.value = 1.0;
    creative_ad.segment = "untargeted";
    creative_ad.geo_targets = {"US"};
    creative_ad.target_url = "https://brave.com/2";
    CreativeDaypartInfo daypart;
    creative_ad.dayparts = {daypart};
    creative_ad.title = "Test Ad 2 Title";
    creative_ad.body = "Test Ad 2 Body";

    return creative_ad;
  }

  void ServeAd() {
    ResetFrequencyCaps(AdType::kAdNotification);

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

TEST_F(BatAdsAdPacingTest, PacingDisableDelivery) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  CreativeAdNotificationInfo creative_ad = BuildCreativeAdNotification1();
  creative_ad.ptr = 0.0;
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  for (const double number : GetAdPacingRandomNumberList()) {
    ScopedAdPacingRandomNumberSetter scoped_setter(number);
    ServeAd();
  }

  // Assert
}

TEST_F(BatAdsAdPacingTest, NoPacing) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  CreativeAdNotificationInfo creative_ad = BuildCreativeAdNotification1();
  creative_ad.ptr = 1.0;
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_))
      .Times(GetAdPacingRandomNumberList().size());

  for (const double number : GetAdPacingRandomNumberList()) {
    ScopedAdPacingRandomNumberSetter scoped_setter(number);
    ServeAd();
  }

  // Assert
}

TEST_F(BatAdsAdPacingTest, SimplePacing) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  CreativeAdNotificationInfo creative_ad = BuildCreativeAdNotification1();
  creative_ad.ptr = 0.5;
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  {
    ScopedAdPacingRandomNumberSetter scoped_setter(0.7);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  {
    ScopedAdPacingRandomNumberSetter scoped_setter(0.3);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(_));
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  // Assert
}

TEST_F(BatAdsAdPacingTest, NoPacingPrioritized) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  const CreativeAdNotificationInfo creative_ad_1 =
      BuildCreativeAdNotification1();
  creative_ads.push_back(creative_ad_1);

  const CreativeAdNotificationInfo creative_ad_2 =
      BuildCreativeAdNotification2();
  creative_ads.push_back(creative_ad_2);

  Save(creative_ads);

  // Act
  for (const double number : GetAdPacingRandomNumberList()) {
    ScopedAdPacingRandomNumberSetter scoped_setter(number);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                       creative_ad_1.creative_instance_id)));
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  // Assert
}

TEST_F(BatAdsAdPacingTest, PacingDisableDeliveryPrioritized) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  CreativeAdNotificationInfo creative_ad_1 = BuildCreativeAdNotification1();
  creative_ad_1.ptr = 0.0;
  creative_ads.push_back(creative_ad_1);

  const CreativeAdNotificationInfo creative_ad_2 =
      BuildCreativeAdNotification2();
  creative_ads.push_back(creative_ad_2);

  Save(creative_ads);

  // Act
  for (const double number : GetAdPacingRandomNumberList()) {
    ScopedAdPacingRandomNumberSetter scoped_setter(number);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                       creative_ad_2.creative_instance_id)));
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  // Assert
}

TEST_F(BatAdsAdPacingTest, PacingAndPrioritization) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  CreativeAdNotificationInfo creative_ad_1 = BuildCreativeAdNotification1();
  creative_ad_1.ptr = 0.4;
  creative_ads.push_back(creative_ad_1);

  CreativeAdNotificationInfo creative_ad_2 = BuildCreativeAdNotification2();
  creative_ad_2.ptr = 0.6;
  creative_ads.push_back(creative_ad_2);

  Save(creative_ads);

  // Act
  {
    ScopedAdPacingRandomNumberSetter scoped_setter(0.1);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                       creative_ad_1.creative_instance_id)));
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  {
    ScopedAdPacingRandomNumberSetter scoped_setter(0.5);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                       creative_ad_2.creative_instance_id)));
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  {
    ScopedAdPacingRandomNumberSetter scoped_setter(0.8);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  // Assert
}

}  // namespace ads
