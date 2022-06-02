/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_v1.h"

#include <memory>

#include "bat/ads/internal/base/container_util.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/serving/targeting/user_model_builder_unittest_util.h"
#include "bat/ads/internal/serving/targeting/user_model_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsEligibleNotificationAdsV1Test : public UnitTestBase {
 protected:
  BatAdsEligibleNotificationAdsV1Test()
      : database_table_(
            std::make_unique<database::table::CreativeNotificationAds>()) {}

  ~BatAdsEligibleNotificationAdsV1Test() override = default;

  void Save(const CreativeNotificationAdList& creative_ads) {
    database_table_->Save(creative_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeNotificationAds> database_table_;
};

TEST_F(BatAdsEligibleNotificationAdsV1Test, GetAdsForChildSegment) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 = BuildCreativeNotificationAd();
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 = BuildCreativeNotificationAd();
  creative_ad_2.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_2);

  Save(creative_ads);

  // Act
  geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  notification_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNotificationAdList expected_creative_ads = {creative_ad_2};

  eligible_ads.GetForUserModel(
      targeting::BuildUserModel({"technology & computing-software"}, {}, {}),
      [&expected_creative_ads](const bool success,
                               const CreativeNotificationAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleNotificationAdsV1Test, GetAdsForParentSegment) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ad.segment = "technology & computing";
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  notification_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNotificationAdList expected_creative_ads = {creative_ad};

  eligible_ads.GetForUserModel(
      targeting::BuildUserModel({"technology & computing-software"}, {}, {}),
      [&expected_creative_ads](const bool success,
                               const CreativeNotificationAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleNotificationAdsV1Test, GetAdsForUntargetedSegment) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ad.segment = "untargeted";
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  notification_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNotificationAdList expected_creative_ads = {creative_ad};

  eligible_ads.GetForUserModel(
      targeting::BuildUserModel({"finance-banking"}, {}, {}),
      [&expected_creative_ads](const bool success,
                               const CreativeNotificationAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleNotificationAdsV1Test, GetAdsForMultipleSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 = BuildCreativeNotificationAd();
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 = BuildCreativeNotificationAd();
  creative_ad_2.segment = "finance-banking";
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 = BuildCreativeNotificationAd();
  creative_ad_3.segment = "food & drink";
  creative_ads.push_back(creative_ad_3);

  Save(creative_ads);

  // Act
  geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  notification_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNotificationAdList expected_creative_ads = {creative_ad_1,
                                                            creative_ad_3};

  eligible_ads.GetForUserModel(
      targeting::BuildUserModel({"technology & computing", "food & drink"}, {},
                                {}),
      [&expected_creative_ads](const bool success,
                               const CreativeNotificationAdList& creative_ads) {
        EXPECT_TRUE(CompareAsSets(expected_creative_ads, creative_ads));
      });

  // Assert
}

TEST_F(BatAdsEligibleNotificationAdsV1Test, GetAdsForNoSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ad.segment = "untargeted";
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  notification_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNotificationAdList expected_creative_ads = {creative_ad};

  eligible_ads.GetForUserModel(
      {},
      [&expected_creative_ads](const bool success,
                               const CreativeNotificationAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleNotificationAdsV1Test, GetAdsForUnmatchedSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ad.segment = "technology & computing";
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  notification_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNotificationAdList expected_creative_ads = {};

  eligible_ads.GetForUserModel(
      targeting::BuildUserModel({"UNMATCHED"}, {}, {}),
      [&expected_creative_ads](const bool success,
                               const CreativeNotificationAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

}  // namespace ads
