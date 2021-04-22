/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;
using ::testing::AllOf;
using ::testing::Between;
using ::testing::Field;
using ::testing::Matcher;

namespace ads {
namespace ad_notifications {

namespace {

Matcher<const AdNotificationInfo&> IsNotification(
    const std::string& creative_instance_id) {
  return AllOf(Field("creative_instance_id",
                     &AdNotificationInfo::creative_instance_id,
                     creative_instance_id));
}

}  // namespace

class BatAdsAdNotificationPacingTest : public UnitTestBase {
 protected:
  BatAdsAdNotificationPacingTest()
      : ad_targeting_(std::make_unique<AdTargeting>()),
        subdivision_targeting_(
            std::make_unique<ad_targeting::geographic::SubdivisionTargeting>()),
        anti_targeting_resource_(std::make_unique<resource::AntiTargeting>()),
        ad_serving_(std::make_unique<ad_notifications::AdServing>(
            ad_targeting_.get(),
            subdivision_targeting_.get(),
            anti_targeting_resource_.get())) {}

  ~BatAdsAdNotificationPacingTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    SetupTestAds();
  }

  void TearDown() override { UnitTestBase::TearDown(); }

  void SetupTestAds() {
    CreativeAdNotificationInfo ad_creative_1;
    ad_creative_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
    ad_creative_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
    ad_creative_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
    ad_creative_1.start_at_timestamp = DistantPastAsTimestamp();
    ad_creative_1.end_at_timestamp = DistantFutureAsTimestamp();
    ad_creative_1.daily_cap = 1;
    ad_creative_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
    ad_creative_1.priority = 1;
    ad_creative_1.per_day = 3;
    ad_creative_1.total_max = 4;
    ad_creative_1.segment = "Technology & Computing-Software";
    ad_creative_1.geo_targets = {"US"};
    ad_creative_1.target_url = "https://brave.com";
    ad_creative_1.title = "Test Ad 1 Title";
    ad_creative_1.body = "Test Ad 1 Body";
    ad_creative_1.ptr = 1.0;
    test_creative_notifications_.push_back(ad_creative_1);

    CreativeAdNotificationInfo ad_creative_2;
    ad_creative_2.creative_instance_id = "a1ac44c2-675f-43e6-ab6d-500614cafe63";
    ad_creative_2.creative_set_id = "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
    ad_creative_2.campaign_id = "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
    ad_creative_2.start_at_timestamp = DistantPastAsTimestamp();
    ad_creative_2.end_at_timestamp = DistantFutureAsTimestamp();
    ad_creative_2.daily_cap = 1;
    ad_creative_2.advertiser_id = "9a11b60f-e29d-4446-8d1f-318311e36e0a";
    ad_creative_2.priority = 2;
    ad_creative_2.per_day = 3;
    ad_creative_2.total_max = 4;
    ad_creative_2.segment = "Food & Drink";
    ad_creative_2.geo_targets = {"US"};
    ad_creative_2.target_url = "https://brave.com";
    ad_creative_2.title = "Test Ad 2 Title";
    ad_creative_2.body = "Test Ad 2 Body";
    ad_creative_2.ptr = 1.0;
    test_creative_notifications_.push_back(ad_creative_2);
  }

  std::unique_ptr<AdTargeting> ad_targeting_;
  std::unique_ptr<ad_targeting::geographic::SubdivisionTargeting>
      subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<ad_notifications::AdServing> ad_serving_;

  std::vector<CreativeAdNotificationInfo> test_creative_notifications_;
};

TEST_F(BatAdsAdNotificationPacingTest, PacingDisableDelivery) {
  CreativeAdNotificationList creative_ad_notifications;
  test_creative_notifications_[0].ptr = 0;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);

  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  for (int i = 0; i < iterations; i++) {
    ad_serving_->MaybeServeAd(
        creative_ad_notifications,
        [](const Result result, const AdNotificationInfo& ad) {});
  }
}

TEST_F(BatAdsAdNotificationPacingTest, NoPacing) {
  CreativeAdNotificationList creative_ad_notifications;
  test_creative_notifications_[0].ptr = 1.0;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);

  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(iterations);

  for (int i = 0; i < iterations; i++) {
    ad_serving_->MaybeServeAd(
        creative_ad_notifications,
        [](const Result result, const AdNotificationInfo& ad) {});
  }
}

TEST_F(BatAdsAdNotificationPacingTest, SimplePacing) {
  CreativeAdNotificationList creative_ad_notifications;
  test_creative_notifications_[0].ptr = 0.2;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);

  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_))
      .Times(Between(iterations * test_creative_notifications_[0].ptr * 0.8,
                     iterations * test_creative_notifications_[0].ptr * 1.2));

  for (int i = 0; i < iterations; i++) {
    ad_serving_->MaybeServeAd(
        creative_ad_notifications,
        [](const Result result, const AdNotificationInfo& ad) {});
  }
}

TEST_F(BatAdsAdNotificationPacingTest, NoPacingPrioritized) {
  CreativeAdNotificationList creative_ad_notifications;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);
  creative_ad_notifications.push_back(test_creative_notifications_[1]);

  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(IsNotification(
                  test_creative_notifications_[0].creative_instance_id)))
      .Times(1);

  ad_serving_->MaybeServeAd(
      creative_ad_notifications,
      [](const Result result, const AdNotificationInfo& ad) {});
}

TEST_F(BatAdsAdNotificationPacingTest, PacingDisableDeliveryPrioritized) {
  CreativeAdNotificationList creative_ad_notifications;
  test_creative_notifications_[0].ptr = 0;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);
  creative_ad_notifications.push_back(test_creative_notifications_[1]);

  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(IsNotification(
                  test_creative_notifications_[1].creative_instance_id)))
      .Times(1);

  ad_serving_->MaybeServeAd(
      creative_ad_notifications,
      [](const Result result, const AdNotificationInfo& ad) {});
}

TEST_F(BatAdsAdNotificationPacingTest, PacingAndPrioritization) {
  CreativeAdNotificationList creative_ad_notifications;
  test_creative_notifications_[0].ptr = 0.5;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);
  test_creative_notifications_[1].ptr = 0.5;
  creative_ad_notifications.push_back(test_creative_notifications_[1]);

  int iterations = 1000;

  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(IsNotification(
                  test_creative_notifications_[0].creative_instance_id)))
      .Times(Between(iterations * test_creative_notifications_[0].ptr * 0.8,
                     iterations * test_creative_notifications_[0].ptr * 1.2));

  // test_creative_notifications_[1] ad would be shown probabilistically when
  // test_creative_notifications_[0] gets dropped due to pacing.
  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(IsNotification(
                  test_creative_notifications_[1].creative_instance_id)))
      .Times(Between(iterations * test_creative_notifications_[0].ptr * 0.8 *
                         test_creative_notifications_[1].ptr,
                     iterations * test_creative_notifications_[0].ptr * 1.2 *
                         test_creative_notifications_[1].ptr));

  for (int i = 0; i < iterations; i++) {
    ad_serving_->MaybeServeAd(
        creative_ad_notifications,
        [](const Result result, const AdNotificationInfo& ad) {});
  }
}

}  // namespace ad_notifications
}  // namespace ads
