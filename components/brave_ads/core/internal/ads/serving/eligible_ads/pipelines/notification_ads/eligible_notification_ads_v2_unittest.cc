/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_v2.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::notification_ads {

class BatAdsEligibleNotificationAdsV2Test : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
    eligible_ads_ = std::make_unique<EligibleAdsV2>(*subdivision_targeting_,
                                                    *anti_targeting_resource_);
  }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<EligibleAdsV2> eligible_ads_;
};

TEST_F(BatAdsEligibleNotificationAdsV2Test, GetAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_1.segment = "foo-bar1";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_2.segment = "foo-bar3";
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({/*interest_segments*/ "foo-bar3"},
                                /*latent_interest_segments*/ {},
                                {"foo-bar1", "foo-bar2"},
                                /*text_embedding_html_events*/ {}),
      base::BindOnce([](const bool had_opportunity,
                        const CreativeNotificationAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(had_opportunity);
        EXPECT_TRUE(!creative_ads.empty());
      }));
}

TEST_F(BatAdsEligibleNotificationAdsV2Test, GetAdsForNoSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_1.segment = "foo";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_2.segment = "foo-bar";
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel(/*interest_segments*/ {},
                                /*latent_interest_segments*/ {},
                                /*purchase_intent_segments*/ {},
                                /*text_embedding_html_events*/ {}),
      base::BindOnce([](const bool had_opportunity,
                        const CreativeNotificationAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(had_opportunity);
        EXPECT_TRUE(!creative_ads.empty());
      }));
}

TEST_F(BatAdsEligibleNotificationAdsV2Test, DoNotGetAdsIfNoEligibleAds) {
  // Arrange

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel(
          {/*interest_segments*/ "interest-foo", "interest-bar"},
          /*latent_interest_segments*/ {}, {"intent-foo", "intent-bar"},
          /*text_embedding_html_events*/ {}),
      base::BindOnce([](const bool had_opportunity,
                        const CreativeNotificationAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

}  // namespace brave_ads::notification_ads
