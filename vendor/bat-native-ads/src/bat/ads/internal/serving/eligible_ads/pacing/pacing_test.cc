/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/net/http/http_status_code.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/internal/base/unittest/unittest_time_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/serving/eligible_ads/eligible_ads_unittest_util.h"
#include "bat/ads/internal/serving/eligible_ads/pacing/pacing_random_util.h"
#include "bat/ads/internal/serving/notification_ad_serving.h"
#include "bat/ads/internal/serving/permission_rules/user_activity_permission_rule_unittest_util.h"
#include "bat/ads/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;
using ::testing::AllOf;
using ::testing::Field;
using ::testing::Matcher;

namespace ads {

namespace {

Matcher<const NotificationAdInfo&> DoesMatchCreativeInstanceId(
    const std::string& creative_instance_id) {
  return AllOf(Field("creative_instance_id",
                     &NotificationAdInfo::creative_instance_id,
                     creative_instance_id));
}

std::vector<double> GetPacingRandomNumberList() {
  return std::vector<double>{0.0, 0.5, 0.99};
}

}  // namespace

class BatAdsPacingIntegrationTest : public UnitTestBase {
 protected:
  BatAdsPacingIntegrationTest()
      : database_table_(
            std::make_unique<database::table::CreativeNotificationAds>()) {}

  ~BatAdsPacingIntegrationTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* is_integration_test */ true);

    ForceUserActivityPermissionRule();
  }

  void SetUpMocks() override {
    CopyFileFromTestPathToTempPath("confirmations_with_unblinded_tokens.json",
                                   kConfirmationsFilename);

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
  }

  CreativeNotificationAdInfo BuildCreativeNotificationAd1() {
    CreativeNotificationAdInfo creative_ad;

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
    creative_ad.target_url = GURL("https://brave.com/1");
    CreativeDaypartInfo daypart;
    creative_ad.dayparts = {daypart};
    creative_ad.title = "Test Ad 1 Title";
    creative_ad.body = "Test Ad 1 Body";

    return creative_ad;
  }

  CreativeNotificationAdInfo BuildCreativeNotificationAd2() {
    CreativeNotificationAdInfo creative_ad;

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
    creative_ad.target_url = GURL("https://brave.com/2");
    CreativeDaypartInfo daypart;
    creative_ad.dayparts = {daypart};
    creative_ad.title = "Test Ad 2 Title";
    creative_ad.body = "Test Ad 2 Body";

    return creative_ad;
  }

  void ServeAd() {
    ResetEligibleAds(AdType::kNotificationAd);

    geographic::SubdivisionTargeting subdivision_targeting;
    resource::AntiTargeting anti_targeting_resource;
    notification_ads::Serving serving(&subdivision_targeting,
                                      &anti_targeting_resource);

    serving.MaybeServeAd();
  }

  void Save(const CreativeNotificationAdList& creative_ads) {
    database_table_->Save(creative_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeNotificationAds> database_table_;
};

TEST_F(BatAdsPacingIntegrationTest, PacingDisableDelivery) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd1();
  creative_ad.ptr = 0.0;
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  for (const double number : GetPacingRandomNumberList()) {
    ScopedPacingRandomNumberSetter scoped_setter(number);
    ServeAd();
  }

  // Assert
}

TEST_F(BatAdsPacingIntegrationTest, NoPacing) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd1();
  creative_ad.ptr = 1.0;
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_))
      .Times(GetPacingRandomNumberList().size());

  for (const double number : GetPacingRandomNumberList()) {
    ScopedPacingRandomNumberSetter scoped_setter(number);
    ServeAd();
  }

  // Assert
}

TEST_F(BatAdsPacingIntegrationTest, SimplePacing) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd1();
  creative_ad.ptr = 0.5;
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  {
    ScopedPacingRandomNumberSetter scoped_setter(0.7);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  {
    ScopedPacingRandomNumberSetter scoped_setter(0.3);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(_));
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  // Assert
}

TEST_F(BatAdsPacingIntegrationTest, NoPacingPrioritized) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  const CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd1();
  creative_ads.push_back(creative_ad_1);

  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd2();
  creative_ads.push_back(creative_ad_2);

  Save(creative_ads);

  // Act
  for (const double number : GetPacingRandomNumberList()) {
    ScopedPacingRandomNumberSetter scoped_setter(number);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                       creative_ad_1.creative_instance_id)));
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  // Assert
}

TEST_F(BatAdsPacingIntegrationTest, PacingDisableDeliveryPrioritized) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 = BuildCreativeNotificationAd1();
  creative_ad_1.ptr = 0.0;
  creative_ads.push_back(creative_ad_1);

  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd2();
  creative_ads.push_back(creative_ad_2);

  Save(creative_ads);

  // Act
  for (const double number : GetPacingRandomNumberList()) {
    ScopedPacingRandomNumberSetter scoped_setter(number);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                       creative_ad_2.creative_instance_id)));
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  // Assert
}

TEST_F(BatAdsPacingIntegrationTest, PacingAndPrioritization) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 = BuildCreativeNotificationAd1();
  creative_ad_1.ptr = 0.4;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 = BuildCreativeNotificationAd2();
  creative_ad_2.ptr = 0.6;
  creative_ads.push_back(creative_ad_2);

  Save(creative_ads);

  // Act
  {
    ScopedPacingRandomNumberSetter scoped_setter(0.1);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                       creative_ad_1.creative_instance_id)));
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  {
    ScopedPacingRandomNumberSetter scoped_setter(0.5);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(DoesMatchCreativeInstanceId(
                                       creative_ad_2.creative_instance_id)));
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  {
    ScopedPacingRandomNumberSetter scoped_setter(0.8);
    EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);
    ServeAd();
    ::testing::Mock::VerifyAndClearExpectations(ads_client_mock_.get());
  }

  // Assert
}

}  // namespace ads
