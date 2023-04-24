/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_v2.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEligibleNewTabPageAdsV2Test : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<AntiTargetingResource>();
    eligible_ads_ = std::make_unique<EligibleNewTabPageAdsV2>(
        *subdivision_targeting_, *anti_targeting_resource_);
  }

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AntiTargetingResource> anti_targeting_resource_;
  std::unique_ptr<EligibleNewTabPageAdsV2> eligible_ads_;
};

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, GetAds) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 =
      BuildCreativeNewTabPageAd(/*should_use_random_guids*/ true);
  creative_ad_1.segment = "foo-bar1";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 =
      BuildCreativeNewTabPageAd(/*should_use_random_guids*/ true);
  creative_ad_2.segment = "foo-bar3";
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModel({/*interest_segments*/ "foo-bar3"},
                     /*latent_interest_segments*/ {}, {"foo-bar1", "foo-bar2"},
                     /*text_embedding_html_events*/ {}),
      base::BindOnce([](const bool had_opportunity,
                        const CreativeNewTabPageAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(had_opportunity);
        EXPECT_FALSE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, GetAdsForNoSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 =
      BuildCreativeNewTabPageAd(/*should_use_random_guids*/ true);
  creative_ad_1.segment = "foo";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 =
      BuildCreativeNewTabPageAd(/*should_use_random_guids*/ true);
  creative_ad_2.segment = "foo-bar";
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModel(/*interest_segments*/ {},
                     /*latent_interest_segments*/ {},
                     /*purchase_intent_segments*/ {},
                     /*text_embedding_html_events*/ {}),
      base::BindOnce([](const bool had_opportunity,
                        const CreativeNewTabPageAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(had_opportunity);
        EXPECT_FALSE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, DoNotGetAdsIfNoEligibleAds) {
  // Arrange

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModel({/*interest_segments*/ "interest-foo", "interest-bar"},
                     /*latent_interest_segments*/ {},
                     {"intent-foo", "intent-bar"},
                     /*text_embedding_html_events*/ {}),
      base::BindOnce([](const bool had_opportunity,
                        const CreativeNewTabPageAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

}  // namespace brave_ads
