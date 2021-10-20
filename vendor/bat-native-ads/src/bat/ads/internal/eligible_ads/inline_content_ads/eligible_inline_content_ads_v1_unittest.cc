/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_v1.h"

#include <memory>

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_builder_unittest_util.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_unittest_util.h"
#include "bat/ads/internal/database/tables/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsEligibleInlineContentAdsV1Test : public UnitTestBase {
 protected:
  BatAdsEligibleInlineContentAdsV1Test()
      : database_table_(
            std::make_unique<database::table::CreativeInlineContentAds>()) {}

  ~BatAdsEligibleInlineContentAdsV1Test() override = default;

  void RecordUserActivityEvents() {
    UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
    UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  }

  void Save(const CreativeInlineContentAdList& creative_ads) {
    database_table_->Save(creative_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeInlineContentAds> database_table_;
};

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForParentChildSegment) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      GetCreativeInlineContentAdForSegment("technology & computing");
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      GetCreativeInlineContentAdForSegment("technology & computing-software");
  creative_ads.push_back(creative_ad_2);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                                 &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_ads = {creative_ad_2};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel({"technology & computing-software"}, {}, {}),
      "200x100",
      [&expected_creative_ads](
          const bool success, const CreativeInlineContentAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForParentSegment) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad =
      GetCreativeInlineContentAdForSegment("technology & computing");
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                                 &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_ads = {creative_ad};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel({"technology & computing-software"}, {}, {}),
      "200x100",
      [&expected_creative_ads](
          const bool success, const CreativeInlineContentAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForUntargetedSegment) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad =
      GetCreativeInlineContentAdForSegment("untargeted");
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                                 &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_ads = {creative_ad};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel({"finance-banking"}, {}, {}), "200x100",
      [&expected_creative_ads](
          const bool success, const CreativeInlineContentAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForMultipleSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      GetCreativeInlineContentAdForSegment("technology & computing");
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      GetCreativeInlineContentAdForSegment("finance-banking");
  creative_ads.push_back(creative_ad_2);

  CreativeInlineContentAdInfo creative_ad_3 =
      GetCreativeInlineContentAdForSegment("food & drink");
  creative_ads.push_back(creative_ad_3);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                                 &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_ads = {creative_ad_1,
                                                             creative_ad_2};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel({"technology & computing", "food & drink"},
                                   {}, {}),
      "200x100",
      [&expected_creative_ads](
          const bool success, const CreativeInlineContentAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForNoSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad =
      GetCreativeInlineContentAdForSegment("untargeted");
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                                 &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_ads = {creative_ad};

  eligible_ads.GetForUserModel(
      {}, "200x100",
      [&expected_creative_ads](
          const bool success, const CreativeInlineContentAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForUnmatchedSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad =
      GetCreativeInlineContentAdForSegment("technology & computing");
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                                 &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_ads = {};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel({"UNMATCHED"}, {}, {}), "200x100",
      [&expected_creative_ads](
          const bool success, const CreativeInlineContentAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForUnmatchedDimensions) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad =
      GetCreativeInlineContentAdForSegment("technology & computing");
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                                 &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_ads = {};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel({"technology & computing"}, {}, {}), "?x?",
      [&expected_creative_ads](
          const bool success, const CreativeInlineContentAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

}  // namespace ads
