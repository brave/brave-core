/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_v2.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEligibleInlineContentAdsV2Test : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<AntiTargetingResource>();
    eligible_ads_ = std::make_unique<EligibleInlineContentAdsV2>(
        *subdivision_targeting_, *anti_targeting_resource_);
  }

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AntiTargetingResource> anti_targeting_resource_;
  std::unique_ptr<EligibleInlineContentAdsV2> eligible_ads_;
};

TEST_F(BraveAdsEligibleInlineContentAdsV2Test, GetAds) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "foo-bar1";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "foo-bar3";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {"foo-bar1", "foo-bar2"},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"foo-bar3"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce([](const bool had_opportunity,
                        const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(had_opportunity);
        EXPECT_FALSE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsEligibleInlineContentAdsV2Test, GetAdsForNoSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "foo";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "foo-bar";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce([](const bool had_opportunity,
                        const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(had_opportunity);
        EXPECT_FALSE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsEligibleInlineContentAdsV2Test,
       DoNotGetAdsForNonExistentDimensions) {
  // Arrange

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {"intent-foo", "intent-bar"},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"interest-foo", "interest-bar"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "?x?",
      base::BindOnce([](const bool had_opportunity,
                        const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsEligibleInlineContentAdsV2Test, DoNotGetAdsIfNoEligibleAds) {
  // Arrange

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {"intent-foo", "intent-bar"},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {"interest-foo", "interest-bar"},
          /*text_embedding_html_events*/ {}),
      /*dimensions*/ "200x100",
      base::BindOnce([](const bool had_opportunity,
                        const CreativeInlineContentAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

}  // namespace brave_ads
