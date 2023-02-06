/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::permission_rules::features {

TEST(BatAdsPermissionRuleFeaturesTest, IsEnabled) {
  // Arrange

  // Act
  const bool is_enabled = IsEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsPermissionRuleFeaturesTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool is_enabled = IsEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
}

TEST(BatAdsPermissionRuleFeaturesTest, ShouldOnlyServeAdsInWindowedMode) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_in_windowed_mode"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_in_windowed_mode =
      ShouldOnlyServeAdsInWindowedMode();

  // Assert
  const bool expected_should_only_serve_ads_in_windowed_mode = false;
  EXPECT_EQ(expected_should_only_serve_ads_in_windowed_mode,
            should_only_serve_ads_in_windowed_mode);
}

TEST(BatAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsInWindowedMode) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool should_only_serve_ads_in_windowed_mode =
      ShouldOnlyServeAdsInWindowedMode();

  // Assert
  const bool expected_should_only_serve_ads_in_windowed_mode = true;
  EXPECT_EQ(expected_should_only_serve_ads_in_windowed_mode,
            should_only_serve_ads_in_windowed_mode);
}

TEST(BatAdsPermissionRuleFeaturesTest, ShouldExcludeAdIfConvertedWhenDisable) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_in_windowed_mode =
      ShouldOnlyServeAdsInWindowedMode();

  // Assert
  const bool expected_should_only_serve_ads_in_windowed_mode = true;
  EXPECT_EQ(expected_should_only_serve_ads_in_windowed_mode,
            should_only_serve_ads_in_windowed_mode);
}

TEST(BatAdsPermissionRuleFeaturesTest,
     ShouldOnlyServeAdsWithValidInternetConnection) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_with_valid_internet_connection"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_with_valid_internet_connection =
      ShouldOnlyServeAdsWithValidInternetConnection();

  // Assert
  const bool expected_should_only_serve_ads_with_valid_internet_connection =
      false;
  EXPECT_EQ(expected_should_only_serve_ads_with_valid_internet_connection,
            should_only_serve_ads_with_valid_internet_connection);
}

TEST(BatAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsWithValidInternetConnection) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool should_only_serve_ads_with_valid_internet_connection =
      ShouldOnlyServeAdsWithValidInternetConnection();

  // Assert
  const bool expected_should_only_serve_ads_with_valid_internet_connection =
      true;
  EXPECT_EQ(expected_should_only_serve_ads_with_valid_internet_connection,
            should_only_serve_ads_with_valid_internet_connection);
}

TEST(BatAdsPermissionRuleFeaturesTest,
     ShouldOnlyServeAdsWithValidInternetConnectionWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_with_valid_internet_connection =
      ShouldOnlyServeAdsWithValidInternetConnection();

  // Assert
  const bool expected_should_only_serve_ads_with_valid_internet_connection =
      true;
  EXPECT_EQ(expected_should_only_serve_ads_with_valid_internet_connection,
            should_only_serve_ads_with_valid_internet_connection);
}

TEST(BatAdsPermissionRuleFeaturesTest, ShouldOnlyServeAdsIfMediaIsNotPlaying) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_if_media_is_not_playing"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_if_media_is_not_playing =
      ShouldOnlyServeAdsIfMediaIsNotPlaying();

  // Assert
  const bool expected_should_only_serve_ads_if_media_is_not_playing = false;
  EXPECT_EQ(expected_should_only_serve_ads_if_media_is_not_playing,
            should_only_serve_ads_if_media_is_not_playing);
}

TEST(BatAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsIfMediaIsNotPlaying) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool should_only_serve_ads_if_media_is_not_playing =
      ShouldOnlyServeAdsIfMediaIsNotPlaying();

  // Assert
  const bool expected_should_only_serve_ads_if_media_is_not_playing = true;
  EXPECT_EQ(expected_should_only_serve_ads_if_media_is_not_playing,
            should_only_serve_ads_if_media_is_not_playing);
}

TEST(BatAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsIfMediaIsNotPlayingWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_if_media_is_not_playing =
      ShouldOnlyServeAdsIfMediaIsNotPlaying();

  // Assert
  const bool expected_should_only_serve_ads_if_media_is_not_playing = true;
  EXPECT_EQ(expected_should_only_serve_ads_if_media_is_not_playing,
            should_only_serve_ads_if_media_is_not_playing);
}

TEST(BatAdsPermissionRuleFeaturesTest, ShouldOnlyServeAdsIfBrowserIsActive) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_if_browser_is_active"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_if_browser_is_active =
      ShouldOnlyServeAdsIfBrowserIsActive();

  // Assert
  const bool expected_should_only_serve_ads_if_browser_is_active = false;
  EXPECT_EQ(expected_should_only_serve_ads_if_browser_is_active,
            should_only_serve_ads_if_browser_is_active);
}

TEST(BatAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsIfBrowserIsActive) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool should_only_serve_ads_if_browser_is_active =
      ShouldOnlyServeAdsIfBrowserIsActive();

  // Assert
  const bool expected_should_only_serve_ads_if_browser_is_active = true;
  EXPECT_EQ(expected_should_only_serve_ads_if_browser_is_active,
            should_only_serve_ads_if_browser_is_active);
}

TEST(BatAdsPermissionRuleFeaturesTest,
     DefaultShouldOnlyServeAdsIfBrowserIsActiveWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_if_browser_is_active =
      ShouldOnlyServeAdsIfBrowserIsActive();

  // Assert
  const bool expected_should_only_serve_ads_if_browser_is_active = true;
  EXPECT_EQ(expected_should_only_serve_ads_if_browser_is_active,
            should_only_serve_ads_if_browser_is_active);
}

}  // namespace ads::permission_rules::features
