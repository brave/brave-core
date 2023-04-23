/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/dislike_exclusion_rule.h"

#include "brave/components/brave_ads/core/ad_content_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDislikeExclusionRuleTest : public UnitTestBase {
 protected:
  DislikeExclusionRule exclusion_rule_;
};

TEST_F(BraveAdsDislikeExclusionRuleTest, AllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = kAdvertiserId;

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDislikeExclusionRuleTest, DoNotAllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = kAdvertiserId;

  AdContentInfo ad_content;
  ad_content.advertiser_id = kAdvertiserId;
  ad_content.like_action_type = AdContentLikeActionType::kNeutral;
  ClientStateManager::GetInstance().ToggleDislikeAd(ad_content);

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
