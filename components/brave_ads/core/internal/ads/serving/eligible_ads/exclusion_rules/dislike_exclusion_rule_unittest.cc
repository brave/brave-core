/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/dislike_exclusion_rule.h"

#include "brave/components/brave_ads/core/ad_content_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsDislikeExclusionRuleTest : public UnitTestBase {};

TEST_F(BatAdsDislikeExclusionRuleTest, AllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = kAdvertiserId;

  // Act
  DislikeExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDislikeExclusionRuleTest, DoNotAllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = kAdvertiserId;

  AdContentInfo ad_content;
  ad_content.advertiser_id = kAdvertiserId;
  ad_content.like_action_type = AdContentLikeActionType::kNeutral;
  ClientStateManager::GetInstance()->ToggleDislikeAd(ad_content);

  // Act
  DislikeExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace brave_ads
