/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsPermissionRuleFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kPermissionRulesFeature));
}

TEST(BraveAdsPermissionRuleFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPermissionRulesFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kPermissionRulesFeature));
}

TEST(BraveAdsPermissionRuleFeatureTest, ShouldOnlyServeAdsInWindowedMode) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPermissionRulesFeature,
      {{"should_only_serve_ads_in_windowed_mode", "false"}});

  // Act & Assert
  EXPECT_FALSE(kShouldOnlyServeAdsInWindowedMode.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest,
     DefaultShouldOnlyServeAdsInWindowedMode) {
  // Act & Assert
  EXPECT_TRUE(kShouldOnlyServeAdsInWindowedMode.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest,
     DefaultShouldOnlyServeAdsInWindowedModeWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPermissionRulesFeature);

  // Act & Assert
  EXPECT_TRUE(kShouldOnlyServeAdsInWindowedMode.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest,
     ShouldOnlyServeAdsWithValidInternetConnection) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPermissionRulesFeature,
      {{"should_only_serve_ads_with_valid_internet_connection", "false"}});

  // Act & Assert
  EXPECT_FALSE(kShouldOnlyServeAdsWithValidInternetConnection.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest,
     DefaultShouldOnlyServeAdsWithValidInternetConnection) {
  // Act & Assert
  EXPECT_TRUE(kShouldOnlyServeAdsWithValidInternetConnection.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest,
     DefaultShouldOnlyServeAdsWithValidInternetConnectionWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPermissionRulesFeature);

  // Act & Assert
  EXPECT_TRUE(kShouldOnlyServeAdsWithValidInternetConnection.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest, ShouldOnlyServeAdsIfMediaIsNotPlaying) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPermissionRulesFeature,
      {{"should_only_serve_ads_if_media_is_not_playing", "false"}});

  // Act & Assert
  EXPECT_FALSE(kShouldOnlyServeAdsIfMediaIsNotPlaying.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest,
     DefaultShouldOnlyServeAdsIfMediaIsNotPlaying) {
  // Act & Assert
  EXPECT_TRUE(kShouldOnlyServeAdsIfMediaIsNotPlaying.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest,
     DefaultShouldOnlyServeAdsIfMediaIsNotPlayingWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPermissionRulesFeature);

  // Act & Assert
  EXPECT_TRUE(kShouldOnlyServeAdsIfMediaIsNotPlaying.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest, ShouldOnlyServeAdsIfBrowserIsActive) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPermissionRulesFeature,
      {{"should_only_serve_ads_if_browser_is_active", "false"}});

  // Act & Assert
  EXPECT_FALSE(kShouldOnlyServeAdsIfBrowserIsActive.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest,
     DefaultShouldOnlyServeAdsIfBrowserIsActive) {
  // Act & Assert
  EXPECT_TRUE(kShouldOnlyServeAdsIfBrowserIsActive.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest,
     DefaultShouldOnlyServeAdsIfBrowserIsActiveWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPermissionRulesFeature);

  // Act & Assert
  EXPECT_TRUE(kShouldOnlyServeAdsIfBrowserIsActive.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest, DoNotDisturbFromHour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPermissionRulesFeature, {{"do_not_disturb_from_hour", "7"}});

  // Act & Assert
  EXPECT_EQ(7, kDoNotDisturbFromHour.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest, DefaultDoNotDisturbFromHour) {
  // Act & Assert
  EXPECT_EQ(21, kDoNotDisturbFromHour.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest,
     DefaultDoNotDisturbFromHourWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPermissionRulesFeature);

  // Act & Assert
  EXPECT_EQ(21, kDoNotDisturbFromHour.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest, DoNotDisturbToHour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPermissionRulesFeature, {{"do_not_disturb_to_hour", "7"}});

  // Act & Assert
  EXPECT_EQ(7, kDoNotDisturbToHour.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest, DefaultDoNotDisturbToHour) {
  // Act & Assert
  EXPECT_EQ(6, kDoNotDisturbToHour.Get());
}

TEST(BraveAdsPermissionRuleFeatureTest, DefaultDoNotDisturbToHourWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPermissionRulesFeature);

  // Act & Assert
  EXPECT_EQ(6, kDoNotDisturbToHour.Get());
}

}  // namespace brave_ads
