/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsPermissionRuleFeaturesTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsPermissionRulesEnabled());
}

TEST(BraveAdsPermissionRuleFeaturesTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kPermissionRulesFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsPermissionRulesEnabled());
}

TEST(BraveAdsPermissionRuleFeaturesTest, ShouldOnlyServeAdsInWindowedMode) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_in_windowed_mode"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kPermissionRulesFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(kShouldOnlyServeAdsInWindowedMode.Get());
}

TEST(BraveAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsInWindowedMode) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(kShouldOnlyServeAdsInWindowedMode.Get());
}

TEST(BraveAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsInWindowedModeWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kPermissionRulesFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(kShouldOnlyServeAdsInWindowedMode.Get());
}

TEST(BraveAdsPermissionRuleFeaturesTest,
     ShouldOnlyServeAdsWithValidInternetConnection) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_with_valid_internet_connection"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kPermissionRulesFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(kShouldOnlyServeAdsWithValidInternetConnection.Get());
}

TEST(BraveAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsWithValidInternetConnection) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(kShouldOnlyServeAdsWithValidInternetConnection.Get());
}

TEST(BraveAdsPermissionRuleFeaturesTest,
     ShouldOnlyServeAdsWithValidInternetConnectionWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kPermissionRulesFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(kShouldOnlyServeAdsWithValidInternetConnection.Get());
}

TEST(BraveAdsPermissionRuleFeaturesTest,
     ShouldOnlyServeAdsIfMediaIsNotPlaying) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_if_media_is_not_playing"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kPermissionRulesFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(kShouldOnlyServeAdsIfMediaIsNotPlaying.Get());
}

TEST(BraveAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsIfMediaIsNotPlaying) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(kShouldOnlyServeAdsIfMediaIsNotPlaying.Get());
}

TEST(BraveAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsIfMediaIsNotPlayingWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kPermissionRulesFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(kShouldOnlyServeAdsIfMediaIsNotPlaying.Get());
}

TEST(BraveAdsPermissionRuleFeaturesTest, ShouldOnlyServeAdsIfBrowserIsActive) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_if_browser_is_active"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kPermissionRulesFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(kShouldOnlyServeAdsIfBrowserIsActive.Get());
}

TEST(BraveAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsIfBrowserIsActive) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(kShouldOnlyServeAdsIfBrowserIsActive.Get());
}

TEST(BraveAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsIfBrowserIsActiveWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kPermissionRulesFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(kShouldOnlyServeAdsIfBrowserIsActive.Get());
}

}  // namespace brave_ads
