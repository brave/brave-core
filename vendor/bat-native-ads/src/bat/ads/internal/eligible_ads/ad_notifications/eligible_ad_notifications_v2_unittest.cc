/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications_v2.h"

#include <memory>

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_builder_unittest_util.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsEligibleAdNotificationsV2Test : public UnitTestBase {
 protected:
  BatAdsEligibleAdNotificationsV2Test()
      : database_table_(
            std::make_unique<database::table::CreativeAdNotifications>()) {}

  ~BatAdsEligibleAdNotificationsV2Test() override = default;

  void RecordUserActivityEvents() {
    UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
    UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  }

  void Save(const CreativeAdNotificationList& creative_ads) {
    database_table_->Save(creative_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeAdNotifications> database_table_;
};

TEST_F(BatAdsEligibleAdNotificationsV2Test, GetAds) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  const CreativeAdNotificationInfo creative_ad_1 =
      GetCreativeAdNotificationForSegment("foo-bar1");
  creative_ads.push_back(creative_ad_1);

  const CreativeAdNotificationInfo creative_ad_2 =
      GetCreativeAdNotificationForSegment("foo-bar3");
  creative_ads.push_back(creative_ad_2);

  Save(creative_ads);

  const SegmentList interest_segments = {"foo-bar3"};
  const SegmentList latent_interest_segments = {};
  const SegmentList purchase_intent_segments = {"foo-bar1", "foo-bar2"};

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAdsV2 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeAdNotificationList expected_creative_ads = {creative_ad_2};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel(interest_segments, latent_interest_segments,
                                   purchase_intent_segments),
      [&expected_creative_ads](const bool had_opportunity,
                               const CreativeAdNotificationList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleAdNotificationsV2Test, GetAdsForNoSegments) {
  // Arrange
  CreativeAdNotificationList creative_ads;

  const CreativeAdNotificationInfo creative_ad_1 =
      GetCreativeAdNotificationForSegment("foo");
  creative_ads.push_back(creative_ad_1);

  const CreativeAdNotificationInfo creative_ad_2 =
      GetCreativeAdNotificationForSegment("foo-bar");
  creative_ads.push_back(creative_ad_2);

  Save(creative_ads);

  const SegmentList interest_segments = {};
  const SegmentList latent_interest_segments = {};
  const SegmentList purchase_intent_segments = {};

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAdsV2 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeAdNotificationList expected_creative_ads = {creative_ad_2};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel(interest_segments, latent_interest_segments,
                                   purchase_intent_segments),
      [&expected_creative_ads](const bool had_opportunity,
                               const CreativeAdNotificationList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleAdNotificationsV2Test, GetIfNoEligibleAds) {
  // Arrange
  const SegmentList interest_segments = {"interest-foo", "interest-bar"};
  const SegmentList latent_interest_segments = {};
  const SegmentList purchase_intent_segments = {"intent-foo", "intent-bar"};

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAdsV2 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeAdNotificationList expected_creative_ads = {};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel(interest_segments, latent_interest_segments,
                                   purchase_intent_segments),
      [&expected_creative_ads](const bool had_opportunity,
                               const CreativeAdNotificationList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

}  // namespace ads
