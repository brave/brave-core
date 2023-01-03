/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_v2.h"

#include <memory>

#include "base/functional/bind.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_builder_unittest_util.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::inline_content_ads {

class BatAdsEligibleInlineContentAdsV2Test : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
    eligible_ads_ = std::make_unique<EligibleAdsV2>(
        subdivision_targeting_.get(), anti_targeting_resource_.get());
  }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<EligibleAdsV2> eligible_ads_;
};

TEST_F(BatAdsEligibleInlineContentAdsV2Test, GetAds) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 = BuildCreativeInlineContentAd();
  creative_ad_1.segment = "foo-bar1";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 = BuildCreativeInlineContentAd();
  creative_ad_2.segment = "foo-bar3";
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({"foo-bar3"}, {}, {"foo-bar1", "foo-bar2"}),
      "200x100",
      base::BindOnce([](const bool had_opportunity,
                        const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(had_opportunity);
        EXPECT_TRUE(!creative_ads.empty());
      }));
}

TEST_F(BatAdsEligibleInlineContentAdsV2Test, GetAdsForNoSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 = BuildCreativeInlineContentAd();
  creative_ad_1.segment = "foo";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 = BuildCreativeInlineContentAd();
  creative_ad_2.segment = "foo-bar";
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({}, {}, {}), "200x100",
      base::BindOnce([](const bool had_opportunity,
                        const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(had_opportunity);
        EXPECT_TRUE(!creative_ads.empty());
      }));
}

TEST_F(BatAdsEligibleInlineContentAdsV2Test,
       DoNotGetAdsForNonExistentDimensions) {
  // Arrange

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({"interest-foo", "interest-bar"}, {},
                                {"intent-foo", "intent-bar"}),
      "?x?",
      base::BindOnce([](const bool had_opportunity,
                        const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BatAdsEligibleInlineContentAdsV2Test, DoNotGetAdsIfNoEligibleAds) {
  // Arrange

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({"interest-foo", "interest-bar"}, {},
                                {"intent-foo", "intent-bar"}),
      "200x100",
      base::BindOnce([](const bool had_opportunity,
                        const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

}  // namespace ads::inline_content_ads
