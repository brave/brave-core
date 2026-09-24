/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/dislike_exclusion_rule.h"

#include <utility>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/tokens/test/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/test/reactions_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDislikeExclusionRuleTest : public test::TestBase {
 protected:
  const DislikeExclusionRule exclusion_rule_;
};

TEST_F(BraveAdsDislikeExclusionRuleTest, ShouldIncludeForNeutralReaction) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = test::kAdvertiserId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsDislikeExclusionRuleTest, ShouldIncludeForLikedReaction) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  base::MockCallback<ResultCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  GetReactions().ToggleLikeAd(
      test::BuildReaction(mojom::AdType::kNotificationAd), callback.Get());

  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = test::kAdvertiserId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsDislikeExclusionRuleTest, ShouldExcludeForDislikedReaction) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  base::MockCallback<ResultCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  GetReactions().ToggleDislikeAd(
      test::BuildReaction(mojom::AdType::kNotificationAd), callback.Get());

  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = test::kAdvertiserId;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad));
}

}  // namespace brave_ads
