/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsExclusionRuleFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kExclusionRulesFeature));
}

TEST(BraveAdsExclusionRuleFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kExclusionRulesFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kExclusionRulesFeature));
}

TEST(BraveAdsExclusionRuleFeatureTest,
     ShouldExcludeAdIfDismissedWithinTimeWindow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "1d"}});

  // Act & Assert
  EXPECT_EQ(base::Days(1), kShouldExcludeAdIfDismissedWithinTimeWindow.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     DefaultShouldExcludeAdIfDismissedWithinTimeWindow) {
  // Act & Assert
  EXPECT_EQ(base::Hours(0), kShouldExcludeAdIfDismissedWithinTimeWindow.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     DefaultShouldExcludeAdIfDismissedWithinTimeWindowWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kExclusionRulesFeature);

  // Act & Assert
  EXPECT_EQ(base::Hours(0), kShouldExcludeAdIfDismissedWithinTimeWindow.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     ShouldExcludeAdIfLandedOnPageWithinTimeWindow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_landed_on_page_within_time_window", "1d"}});

  // Act & Assert
  EXPECT_EQ(base::Days(1),
            kShouldExcludeAdIfLandedOnPageWithinTimeWindow.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     DefaultShouldExcludeAdIfLandedOnPageWithinTimeWindow) {
  // Act & Assert
  EXPECT_EQ(base::Hours(0),
            kShouldExcludeAdIfLandedOnPageWithinTimeWindow.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     DefaultShouldExcludeAdIfLandedOnPageWithinTimeWindowWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kExclusionRulesFeature);

  // Act & Assert
  EXPECT_EQ(base::Hours(0),
            kShouldExcludeAdIfLandedOnPageWithinTimeWindow.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     ShouldExcludeAdIfCreativeInstanceExceedsPerHourCap) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_creative_instance_exceeds_per_hour_cap", "7"}});

  // Act & Assert
  EXPECT_EQ(7, kShouldExcludeAdIfCreativeInstanceExceedsPerHourCap.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     DefaultShouldExcludeAdIfCreativeInstanceExceedsPerHourCap) {
  // Act & Assert
  EXPECT_EQ(1, kShouldExcludeAdIfCreativeInstanceExceedsPerHourCap.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     DefaultShouldExcludeAdIfCreativeInstanceExceedsPerHourCapWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kExclusionRulesFeature);

  // Act & Assert
  EXPECT_EQ(1, kShouldExcludeAdIfCreativeInstanceExceedsPerHourCap.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     ShouldExcludeAdIfCreativeSetExceedsConversionCap) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_creative_set_exceeds_conversion_cap", "7"}});

  // Act & Assert
  EXPECT_EQ(7, kShouldExcludeAdIfCreativeSetExceedsConversionCap.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     DefaultShouldExcludeAdIfCreativeSetExceedsConversionCap) {
  // Act & Assert
  EXPECT_EQ(1, kShouldExcludeAdIfCreativeSetExceedsConversionCap.Get());
}

TEST(BraveAdsExclusionRuleFeatureTest,
     DefaultShouldExcludeAdIfCreativeSetxceedsConversionCapWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kExclusionRulesFeature);

  // Act & Assert
  EXPECT_EQ(1, kShouldExcludeAdIfCreativeSetExceedsConversionCap.Get());
}

}  // namespace brave_ads
