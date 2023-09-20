/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_v1.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_container_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing_random_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/units/inline_content_ad/inline_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEligibleInlineContentAdsV1Test : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<AntiTargetingResource>();
    eligible_ads_ = std::make_unique<EligibleInlineContentAdsV1>(
        *subdivision_targeting_, *anti_targeting_resource_);
  }

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AntiTargetingResource> anti_targeting_resource_;
  std::unique_ptr<EligibleInlineContentAdsV1> eligible_ads_;
};

TEST_F(BraveAdsEligibleInlineContentAdsV1Test, GetAdsForChildSegment) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_2};

  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"technology & computing-software"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsEligibleInlineContentAdsV1Test, GetAdsForParentSegment) {
  // Arrange
  CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad.segment = "technology & computing";
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad};

  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"technology & computing-software"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsEligibleInlineContentAdsV1Test, GetAdsForUntargetedSegment) {
  // Arrange
  CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad.segment = "untargeted";
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad};

  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"finance-banking"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsEligibleInlineContentAdsV1Test, GetAdsForMultipleSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "finance-banking";
  creative_ads.push_back(creative_ad_2);

  CreativeInlineContentAdInfo creative_ad_3 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.segment = "food & drink";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_1,
                                                       creative_ad_3};

  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"technology & computing", "food & drink"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsEligibleInlineContentAdsV1Test, GetAdsForNoSegments) {
  // Arrange
  CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad.segment = "untargeted";
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad};

  eligible_ads_->GetForUserModel(
      /*user_model*/ {}, /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsEligibleInlineContentAdsV1Test,
       DoNotGetAdsForUnmatchedSegments) {
  // Arrange
  CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad.segment = "technology & computing";
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"UNMATCHED"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce([](const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsEligibleInlineContentAdsV1Test,
       DoNotGetAdsForNonExistentDimensions) {
  // Arrange
  CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad.segment = "technology & computing";
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"technology & computing"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "?x?",
      base::BindOnce([](const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsEligibleInlineContentAdsV1Test, DoNotGetAdsIfAlreadySeen) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeInlineContentAds(creative_ads);

  const InlineContentAdInfo ad = BuildInlineContentAd(creative_ad_1);
  ClientStateManager::GetInstance().UpdateSeenAd(ad);

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_2};

  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"technology & computing", "food & drink"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsEligibleInlineContentAdsV1Test, DoNotGetPacedAds) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "technology & computing";
  creative_ad_1.pass_through_rate = 0.1;
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "food & drink";
  creative_ad_2.pass_through_rate = 0.5;
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act
  const ScopedPacingRandomNumberSetterForTesting scoped_setter(0.3);

  CreativeInlineContentAdList expected_creative_ads = {creative_ad_2};

  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"technology & computing", "food & drink"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsEligibleInlineContentAdsV1Test, GetPrioritizedAds) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "technology & computing";
  creative_ad_1.priority = 1;
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "finance-banking";
  creative_ad_2.priority = 1;
  creative_ads.push_back(creative_ad_2);

  CreativeInlineContentAdInfo creative_ad_3 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.segment = "food & drink";
  creative_ad_3.priority = 2;
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_1};

  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"technology & computing", "food & drink"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const CreativeInlineContentAdList& creative_ads) {
            // Assert
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

}  // namespace brave_ads
