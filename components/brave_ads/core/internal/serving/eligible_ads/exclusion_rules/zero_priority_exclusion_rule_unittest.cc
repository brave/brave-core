/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/zero_priority_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsZeroPriorityExclusionRuleTest : public test::TestBase {
 protected:
  const ZeroPriorityExclusionRule exclusion_rule_;
};

TEST_F(BraveAdsZeroPriorityExclusionRuleTest,
       ShouldIncludeIfPriorityIsNonZero) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.priority = 1;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsZeroPriorityExclusionRuleTest, ShouldExcludeIfPriorityIsZero) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.priority = 0;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad));
}

}  // namespace brave_ads
