/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/dislike_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDislikeExclusionRuleTest : public test::TestBase {
 protected:
  const DislikeExclusionRule exclusion_rule_;
};

TEST_F(BraveAdsDislikeExclusionRuleTest, ShouldInclude) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = test::kAdvertiserId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDislikeExclusionRuleTest, ShouldExclude) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = test::kAdvertiserId;

  GetReactions().ToggleDislikeAd(test::kAdvertiserId);

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
