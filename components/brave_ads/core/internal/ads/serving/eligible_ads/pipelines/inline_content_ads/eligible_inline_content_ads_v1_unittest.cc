/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_v1.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pacing/pacing_random_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_container_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::inline_content_ads {

class BatAdsEligibleInlineContentAdsV1Test : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
    eligible_ads_ = std::make_unique<EligibleAdsV1>(
        subdivision_targeting_.get(), anti_targeting_resource_.get());
  }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<EligibleAdsV1> eligible_ads_;
};

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForChildSegment) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_2.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_2};

  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel(
          /*interest_segments*/ {"technology & computing-software"},
          /*latent_interest_segments*/ {},
          /*purchase_intent_segments*/ {},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const bool had_opportunity,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_TRUE(had_opportunity);
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForParentSegment) {
  // Arrange
  CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad.segment = "technology & computing";
  SaveCreativeAds({creative_ad});

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad};

  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel(
          /*interest_segments*/ {"technology & computing-software"},
          /*latent_interest_segments*/ {},
          /*purchase_intent_segments*/ {},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const bool had_opportunity,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_TRUE(had_opportunity);
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForUntargetedSegment) {
  // Arrange
  CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad.segment = "untargeted";
  SaveCreativeAds({creative_ad});

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad};

  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel(/*interest_segments*/ {"finance-banking"},
                                /*latent_interest_segments*/ {},
                                /*purchase_intent_segments*/ {},
                                /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const bool had_opportunity,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_TRUE(had_opportunity);
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForMultipleSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_2.segment = "finance-banking";
  creative_ads.push_back(creative_ad_2);

  CreativeInlineContentAdInfo creative_ad_3 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_3.segment = "food & drink";
  creative_ads.push_back(creative_ad_3);

  SaveCreativeAds(creative_ads);

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_1,
                                                       creative_ad_3};

  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel(
          /*interest_segments*/ {"technology & computing", "food & drink"},
          /*latent_interest_segments*/ {},
          /*purchase_intent_segments*/ {},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const bool had_opportunity,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_TRUE(had_opportunity);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetAdsForNoSegments) {
  // Arrange
  CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad.segment = "untargeted";
  SaveCreativeAds({creative_ad});

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad};

  eligible_ads_->GetForUserModel(
      /*user_model*/ {}, /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const bool had_opportunity,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_TRUE(had_opportunity);
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, DoNotGetAdsForUnmatchedSegments) {
  // Arrange
  CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad.segment = "technology & computing";
  SaveCreativeAds({creative_ad});

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({/*interest_segments*/ "UNMATCHED"},
                                /*latent_interest_segments*/ {},
                                /*purchase_intent_segments*/ {},
                                /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce([](const bool had_opportunity,
                        const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test,
       DoNotGetAdsForNonExistentDimensions) {
  // Arrange
  CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad.segment = "technology & computing";
  SaveCreativeAds({creative_ad});

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel(
          {/*interest_segments*/ "technology & computing"},
          /*latent_interest_segments*/ {},
          /*purchase_intent_segments*/ {},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "?x?",
      base::BindOnce([](const bool had_opportunity,
                        const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, DoNotGetAdsIfAlreadySeen) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  const InlineContentAdInfo ad = BuildInlineContentAd(creative_ad_1);
  ClientStateManager::GetInstance()->UpdateSeenAd(ad);

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_2};

  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel(
          {/*interest_segments*/ "technology & computing", "food & drink"},
          /*latent_interest_segments*/ {},
          /*purchase_intent_segments*/ {},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const bool had_opportunity,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_TRUE(had_opportunity);
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, DoNotGetPacedAds) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_1.segment = "technology & computing";
  creative_ad_1.ptr = 0.1;
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_2.segment = "food & drink";
  creative_ad_2.ptr = 0.5;
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  const ScopedPacingRandomNumberSetter scoped_setter(0.3);

  CreativeInlineContentAdList expected_creative_ads = {creative_ad_2};

  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel(
          {/*interest_segments*/ "technology & computing", "food & drink"},
          /*latent_interest_segments*/ {},
          /*purchase_intent_segments*/ {},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const bool had_opportunity,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_TRUE(had_opportunity);
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsEligibleInlineContentAdsV1Test, GetPrioritizedAds) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_1.segment = "technology & computing";
  creative_ad_1.priority = 1;
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_2.segment = "finance-banking";
  creative_ad_2.priority = 1;
  creative_ads.push_back(creative_ad_2);

  CreativeInlineContentAdInfo creative_ad_3 =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
  creative_ad_3.segment = "food & drink";
  creative_ad_3.priority = 2;
  creative_ads.push_back(creative_ad_3);

  SaveCreativeAds(creative_ads);

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_1};

  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel(
          {/*interest_segments*/ "technology & computing", "food & drink"},
          /*latent_interest_segments*/ {},
          /*purchase_intent_segments*/ {},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const bool had_opportunity,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_TRUE(had_opportunity);
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

}  // namespace brave_ads::inline_content_ads
