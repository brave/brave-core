/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/dislike_category_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDislikeCategoryExclusionRuleTest : public test::TestBase {
 protected:
  const DislikeCategoryExclusionRule exclusion_rule_;
};

TEST_F(BraveAdsDislikeCategoryExclusionRuleTest, ShouldInclude) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.segment = test::kSegment;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDislikeCategoryExclusionRuleTest, ShouldExclude) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.segment = test::kSegment;

  AdHistoryItemInfo ad_history_item;
  ad_history_item.segment = creative_ad.segment;
  ad_history_item.category_user_reaction_type =
      mojom::UserReactionType::kNeutral;
  ClientStateManager::GetInstance().ToggleDislikeCategory(ad_history_item);

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
