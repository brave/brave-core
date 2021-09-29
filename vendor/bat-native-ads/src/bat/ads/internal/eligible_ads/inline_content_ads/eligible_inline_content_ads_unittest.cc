/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads.h"

#include <memory>

#include "base/guid.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_builder_unittest_util.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/database/tables/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsEligibleInlineContentAdsTest : public UnitTestBase {
 protected:
  BatAdsEligibleInlineContentAdsTest()
      : database_table_(
            std::make_unique<database::table::CreativeInlineContentAds>()) {}

  ~BatAdsEligibleInlineContentAdsTest() override = default;

  void RecordUserActivityEvents() {
    UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
    UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  }

  CreativeInlineContentAdInfo GetCreativeInlineContentAdForSegment(
      const std::string& segment) {
    CreativeInlineContentAdInfo creative_inline_content_ad;

    creative_inline_content_ad.creative_instance_id = base::GenerateGUID();
    creative_inline_content_ad.creative_set_id = base::GenerateGUID();
    creative_inline_content_ad.campaign_id = base::GenerateGUID();
    creative_inline_content_ad.start_at = DistantPast();
    creative_inline_content_ad.end_at = DistantFuture();
    creative_inline_content_ad.daily_cap = 1;
    creative_inline_content_ad.advertiser_id = base::GenerateGUID();
    creative_inline_content_ad.priority = 1;
    creative_inline_content_ad.ptr = 1.0;
    creative_inline_content_ad.per_day = 1;
    creative_inline_content_ad.per_week = 1;
    creative_inline_content_ad.per_month = 1;
    creative_inline_content_ad.total_max = 1;
    creative_inline_content_ad.value = 1.0;
    creative_inline_content_ad.segment = segment;
    creative_inline_content_ad.geo_targets = {"US"};
    creative_inline_content_ad.target_url = "https://brave.com";
    CreativeDaypartInfo daypart;
    creative_inline_content_ad.dayparts = {daypart};
    creative_inline_content_ad.title = "Test Ad Title";
    creative_inline_content_ad.description = "Test Ad Description";
    creative_inline_content_ad.image_url = "https://brave.com/image";
    creative_inline_content_ad.dimensions = "200x100";
    creative_inline_content_ad.cta_text = "Call to action text";

    return creative_inline_content_ad;
  }

  void Save(const CreativeInlineContentAdList& creative_inline_content_ads) {
    database_table_->Save(creative_inline_content_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeInlineContentAds> database_table_;
};

TEST_F(BatAdsEligibleInlineContentAdsTest, GetAdsForParentChildSegment) {
  // Arrange
  CreativeInlineContentAdList creative_inline_content_ads;

  CreativeInlineContentAdInfo creative_inline_content_ad_1 =
      GetCreativeInlineContentAdForSegment("technology & computing");
  creative_inline_content_ads.push_back(creative_inline_content_ad_1);

  CreativeInlineContentAdInfo creative_inline_content_ad_2 =
      GetCreativeInlineContentAdForSegment("technology & computing-software");
  creative_inline_content_ads.push_back(creative_inline_content_ad_2);

  Save(creative_inline_content_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAds eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_inline_content_ads = {
      creative_inline_content_ad_2};

  eligible_ads.Get(
      ad_targeting::BuildUserModel({"technology & computing-software"}),
      "200x100",
      [&expected_creative_inline_content_ads](
          const bool success,
          const CreativeInlineContentAdList& creative_inline_content_ads) {
        EXPECT_EQ(expected_creative_inline_content_ads,
                  creative_inline_content_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsTest, GetAdsForParentSegment) {
  // Arrange
  CreativeInlineContentAdList creative_inline_content_ads;

  CreativeInlineContentAdInfo creative_inline_content_ad =
      GetCreativeInlineContentAdForSegment("technology & computing");
  creative_inline_content_ads.push_back(creative_inline_content_ad);

  Save(creative_inline_content_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAds eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_inline_content_ads = {
      creative_inline_content_ad};

  eligible_ads.Get(
      ad_targeting::BuildUserModel({"technology & computing-software"}),
      "200x100",
      [&expected_creative_inline_content_ads](
          const bool success,
          const CreativeInlineContentAdList& creative_inline_content_ads) {
        EXPECT_EQ(expected_creative_inline_content_ads,
                  creative_inline_content_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsTest, GetAdsForUntargetedSegment) {
  // Arrange
  CreativeInlineContentAdList creative_inline_content_ads;

  CreativeInlineContentAdInfo creative_inline_content_ad =
      GetCreativeInlineContentAdForSegment("untargeted");
  creative_inline_content_ads.push_back(creative_inline_content_ad);

  Save(creative_inline_content_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAds eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_inline_content_ads = {
      creative_inline_content_ad};

  eligible_ads.Get(
      ad_targeting::BuildUserModel({"finance-banking"}), "200x100",
      [&expected_creative_inline_content_ads](
          const bool success,
          const CreativeInlineContentAdList& creative_inline_content_ads) {
        EXPECT_EQ(expected_creative_inline_content_ads,
                  creative_inline_content_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsTest, GetAdsForMultipleSegments) {
  // Arrange
  CreativeInlineContentAdList creative_inline_content_ads;

  CreativeInlineContentAdInfo creative_inline_content_ad_1 =
      GetCreativeInlineContentAdForSegment("technology & computing");
  creative_inline_content_ads.push_back(creative_inline_content_ad_1);

  CreativeInlineContentAdInfo creative_inline_content_ad_2 =
      GetCreativeInlineContentAdForSegment("finance-banking");
  creative_inline_content_ads.push_back(creative_inline_content_ad_2);

  CreativeInlineContentAdInfo creative_inline_content_ad_3 =
      GetCreativeInlineContentAdForSegment("food & drink");
  creative_inline_content_ads.push_back(creative_inline_content_ad_3);

  Save(creative_inline_content_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAds eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_inline_content_ads = {
      creative_inline_content_ad_1, creative_inline_content_ad_2};

  eligible_ads.Get(
      ad_targeting::BuildUserModel({"technology & computing", "food & drink"}),
      "200x100",
      [&expected_creative_inline_content_ads](
          const bool success,
          const CreativeInlineContentAdList& creative_inline_content_ads) {
        EXPECT_EQ(expected_creative_inline_content_ads,
                  creative_inline_content_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsTest, GetAdsForUntargetedForNoSegments) {
  // Arrange
  CreativeInlineContentAdList creative_inline_content_ads;

  CreativeInlineContentAdInfo creative_inline_content_ad =
      GetCreativeInlineContentAdForSegment("untargeted");
  creative_inline_content_ads.push_back(creative_inline_content_ad);

  Save(creative_inline_content_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAds eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_inline_content_ads = {
      creative_inline_content_ad};

  eligible_ads.Get(
      {}, "200x100",
      [&expected_creative_inline_content_ads](
          const bool success,
          const CreativeInlineContentAdList& creative_inline_content_ads) {
        EXPECT_EQ(expected_creative_inline_content_ads,
                  creative_inline_content_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsTest, GetAdsForUnmatchedSegments) {
  // Arrange
  CreativeInlineContentAdList creative_inline_content_ads;

  CreativeInlineContentAdInfo creative_inline_content_ad_1 =
      GetCreativeInlineContentAdForSegment("technology & computing");
  creative_inline_content_ads.push_back(creative_inline_content_ad_1);

  Save(creative_inline_content_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAds eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_inline_content_ads = {};

  eligible_ads.Get(
      ad_targeting::BuildUserModel({"UNMATCHED"}), "200x100",
      [&expected_creative_inline_content_ads](
          const bool success,
          const CreativeInlineContentAdList& creative_inline_content_ads) {
        EXPECT_EQ(expected_creative_inline_content_ads,
                  creative_inline_content_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsTest, GetAdsForUnmatchedDimensions) {
  // Arrange
  CreativeInlineContentAdList creative_inline_content_ads;

  CreativeInlineContentAdInfo creative_inline_content_ad_1 =
      GetCreativeInlineContentAdForSegment("technology & computing");
  creative_inline_content_ads.push_back(creative_inline_content_ad_1);

  Save(creative_inline_content_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAds eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeInlineContentAdList expected_creative_inline_content_ads = {};

  eligible_ads.Get(
      ad_targeting::BuildUserModel({"technology & computing"}), "?x?",
      [&expected_creative_inline_content_ads](
          const bool success,
          const CreativeInlineContentAdList& creative_inline_content_ads) {
        EXPECT_EQ(expected_creative_inline_content_ads,
                  creative_inline_content_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsTest, GetV2WithoutAds) {
  // Arrange
  const SegmentList interest_segments = {"interest-foo", "interest-bar"};
  const SegmentList purchase_intent_segments = {"intent-foo", "intent-bar"};
  const ad_targeting::UserModelInfo user_model =
      ad_targeting::BuildUserModel(interest_segments, purchase_intent_segments);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAds eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  eligible_ads.GetV2(
      user_model, "200x100",
      [=](const bool was_allowed,
          const absl::optional<CreativeInlineContentAdInfo>& ad) {
        EXPECT_EQ(absl::nullopt, ad);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsTest, GetV2WithEmptySegments) {
  // Arrange
  CreativeInlineContentAdList creative_inline_content_ads;

  const CreativeInlineContentAdInfo creative_inline_content_ad_1 =
      GetCreativeInlineContentAdForSegment("foo");
  creative_inline_content_ads.push_back(creative_inline_content_ad_1);

  const CreativeInlineContentAdInfo creative_inline_content_ad_2 =
      GetCreativeInlineContentAdForSegment("foo-bar");
  creative_inline_content_ads.push_back(creative_inline_content_ad_2);

  Save(creative_inline_content_ads);

  const SegmentList interest_segments = {};
  const SegmentList purchase_intent_segments = {};
  const ad_targeting::UserModelInfo user_model =
      ad_targeting::BuildUserModel(interest_segments, purchase_intent_segments);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAds eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeInlineContentAdInfo expected_ad = creative_inline_content_ad_2;

  eligible_ads.GetV2(
      user_model, "200x100",
      [=](const bool was_allowed,
          const absl::optional<CreativeInlineContentAdInfo>& ad) {
        EXPECT_TRUE(ad);
      });

  // Assert
}

TEST_F(BatAdsEligibleInlineContentAdsTest, GetV2) {
  // Arrange
  CreativeInlineContentAdList creative_inline_content_ads;

  const CreativeInlineContentAdInfo creative_inline_content_ad_1 =
      GetCreativeInlineContentAdForSegment("foo-bar1");
  creative_inline_content_ads.push_back(creative_inline_content_ad_1);

  const CreativeInlineContentAdInfo creative_inline_content_ad_2 =
      GetCreativeInlineContentAdForSegment("foo-bar3");
  creative_inline_content_ads.push_back(creative_inline_content_ad_2);

  Save(creative_inline_content_ads);

  const SegmentList interest_segments = {"foo-bar3"};
  const SegmentList purchase_intent_segments = {"foo-bar1", "foo-bar2"};
  const ad_targeting::UserModelInfo user_model =
      ad_targeting::BuildUserModel(interest_segments, purchase_intent_segments);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  inline_content_ads::EligibleAds eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeInlineContentAdInfo expected_ad = creative_inline_content_ad_2;

  eligible_ads.GetV2(
      user_model, "200x100",
      [=](const bool was_allowed,
          const absl::optional<CreativeInlineContentAdInfo>& ad) {
        EXPECT_TRUE(ad);
      });

  // Assert
}

}  // namespace ads
