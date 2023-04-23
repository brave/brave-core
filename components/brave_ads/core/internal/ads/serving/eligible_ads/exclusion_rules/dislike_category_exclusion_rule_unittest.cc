/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/dislike_category_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDislikeCategoryExclusionRuleTest : public UnitTestBase {
 protected:
  DislikeCategoryExclusionRule exclusion_rule_;
};

TEST_F(BraveAdsDislikeCategoryExclusionRuleTest, AllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.segment = kSegment;

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDislikeCategoryExclusionRuleTest, DoNotAllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.segment = kSegment;

  ClientStateManager::GetInstance()->ToggleDislikeCategory(
      creative_ad.segment, CategoryContentOptActionType::kNone);

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
