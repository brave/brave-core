/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsExclusionRuleFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kExclusionRulesFeature));
}

TEST(BraveAdsExclusionRuleFeatureTest,
     ShouldExcludeAdIfDismissedWithinTimeWindow) {
  // Act & Assert
  EXPECT_EQ(base::Hours(0), kShouldExcludeAdIfDismissedWithinTimeWindow.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     ShouldExcludeAdIfLandedOnPageWithinTimeWindow) {
  // Act & Assert
  EXPECT_EQ(base::Hours(0),
            kShouldExcludeAdIfLandedOnPageWithinTimeWindow.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     ShouldExcludeAdIfCreativeInstanceExceedsPerHourCap) {
  // Act & Assert
  EXPECT_EQ(1U, kShouldExcludeAdIfCreativeInstanceExceedsPerHourCap.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     ShouldExcludeAdIfCreativeSetExceedsConversionCap) {
  // Act & Assert
  EXPECT_EQ(0U, kShouldExcludeAdIfCreativeSetExceedsConversionCap.Get());
}

}  // namespace brave_ads
