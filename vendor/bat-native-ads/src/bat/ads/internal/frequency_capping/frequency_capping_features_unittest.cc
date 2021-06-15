/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping_features.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsFrequencyCappingFeaturesTest, IsEnabled) {
  // Arrange

  // Act
  const bool is_enabled = features::frequency_capping::IsEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsFrequencyCappingFeaturesTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::frequency_capping::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool is_enabled = features::frequency_capping::IsEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
}

TEST(BatAdsFrequencyCappingFeaturesTest, ShouldExcludeAdIfConverted) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["should_exclude_ad_if_converted"] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_exclude_ad_if_converted =
      features::frequency_capping::ShouldExcludeAdIfConverted();

  // Assert
  const bool expected_should_exclude_ad_if_converted = false;
  EXPECT_EQ(expected_should_exclude_ad_if_converted,
            should_exclude_ad_if_converted);
}

TEST(BatAdsFrequencyCappingFeaturesTest, DefaultShouldExcludeAdIfConverted) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool should_exclude_ad_if_converted =
      features::frequency_capping::ShouldExcludeAdIfConverted();

  // Assert
  const bool expected_should_exclude_ad_if_converted = true;
  EXPECT_EQ(expected_should_exclude_ad_if_converted,
            should_exclude_ad_if_converted);
}

TEST(BatAdsFrequencyCappingFeaturesTest, DisabledShouldExcludeAdIfConverted) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::frequency_capping::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_exclude_ad_if_converted =
      features::frequency_capping::ShouldExcludeAdIfConverted();

  // Assert
  const bool expected_should_exclude_ad_if_converted = true;
  EXPECT_EQ(expected_should_exclude_ad_if_converted,
            should_exclude_ad_if_converted);
}

TEST(BatAdsFrequencyCappingFeaturesTest, ExcludeAdIfDismissedWithinTimeWindow) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kParameter[] = "exclude_ad_if_dismissed_within_time_window";
  parameters[kParameter] = "24h";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window =
      features::frequency_capping::ExcludeAdIfDismissedWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::TimeDelta::FromDays(1);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     DefaultExcludeAdIfDismissedWithinTimeWindow) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window =
      features::frequency_capping::ExcludeAdIfDismissedWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::TimeDelta::FromDays(2);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     DefaultExcludeAdIfDismissedWithinTimeWindowWhenDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::frequency_capping::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window =
      features::frequency_capping::ExcludeAdIfDismissedWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::TimeDelta::FromDays(2);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsUserActivityFeaturesTest, ExcludeAdIfTransferredWithinTimeWindow) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kParameter[] = "exclude_ad_if_transferred_within_time_window";
  parameters[kParameter] = "24h";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window =
      features::frequency_capping::ExcludeAdIfTransferredWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::TimeDelta::FromDays(1);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsUserActivityFeaturesTest,
     DefaultExcludeAdIfTransferredWithinTimeWindow) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window =
      features::frequency_capping::ExcludeAdIfTransferredWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::TimeDelta::FromDays(2);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsUserActivityFeaturesTest,
     DefaultExcludeAdIfTransferredWithinTimeWindowWhenDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::frequency_capping::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window =
      features::frequency_capping::ExcludeAdIfTransferredWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::TimeDelta::FromDays(2);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsFrequencyCappingFeaturesTest, ShouldOnlyServeAdsInWindowedMode) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["should_only_serve_ads_in_windowed_mode"] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_in_windowed_mode =
      features::frequency_capping::ShouldOnlyServeAdsInWindowedMode();

  // Assert
  const bool expected_should_only_serve_ads_in_windowed_mode = false;
  EXPECT_EQ(expected_should_only_serve_ads_in_windowed_mode,
            should_only_serve_ads_in_windowed_mode);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     DefaultShouldOnlyServeAdsInWindowedMode) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool should_only_serve_ads_in_windowed_mode =
      features::frequency_capping::ShouldOnlyServeAdsInWindowedMode();

  // Assert
  const bool expected_should_only_serve_ads_in_windowed_mode = true;
  EXPECT_EQ(expected_should_only_serve_ads_in_windowed_mode,
            should_only_serve_ads_in_windowed_mode);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     ShouldExcludeAdIfConvertedWhenDisable) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::frequency_capping::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_in_windowed_mode =
      features::frequency_capping::ShouldOnlyServeAdsInWindowedMode();

  // Assert
  const bool expected_should_only_serve_ads_in_windowed_mode = true;
  EXPECT_EQ(expected_should_only_serve_ads_in_windowed_mode,
            should_only_serve_ads_in_windowed_mode);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     ShouldOnlyServeAdsWithValidInternetConnection) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["should_only_serve_ads_with_valid_internet_connection"] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_with_valid_internet_connection = features::
      frequency_capping::ShouldOnlyServeAdsWithValidInternetConnection();

  // Assert
  const bool expected_should_only_serve_ads_with_valid_internet_connection =
      false;
  EXPECT_EQ(expected_should_only_serve_ads_with_valid_internet_connection,
            should_only_serve_ads_with_valid_internet_connection);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     DefaultShouldOnlyServeAdsWithValidInternetConnection) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool should_only_serve_ads_with_valid_internet_connection = features::
      frequency_capping::ShouldOnlyServeAdsWithValidInternetConnection();

  // Assert
  const bool expected_should_only_serve_ads_with_valid_internet_connection =
      true;
  EXPECT_EQ(expected_should_only_serve_ads_with_valid_internet_connection,
            should_only_serve_ads_with_valid_internet_connection);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     ShouldOnlyServeAdsWithValidInternetConnectionWhenDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::frequency_capping::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_with_valid_internet_connection = features::
      frequency_capping::ShouldOnlyServeAdsWithValidInternetConnection();

  // Assert
  const bool expected_should_only_serve_ads_with_valid_internet_connection =
      true;
  EXPECT_EQ(expected_should_only_serve_ads_with_valid_internet_connection,
            should_only_serve_ads_with_valid_internet_connection);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     ShouldOnlyServeAdsIfMediaIsNotPlaying) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["should_only_serve_ads_if_media_is_not_playing"] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_if_media_is_not_playing =
      features::frequency_capping::ShouldOnlyServeAdsIfMediaIsNotPlaying();

  // Assert
  const bool expected_should_only_serve_ads_if_media_is_not_playing = false;
  EXPECT_EQ(expected_should_only_serve_ads_if_media_is_not_playing,
            should_only_serve_ads_if_media_is_not_playing);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     DefaultShouldOnlyServeAdsIfMediaIsNotPlaying) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool should_only_serve_ads_if_media_is_not_playing =
      features::frequency_capping::ShouldOnlyServeAdsIfMediaIsNotPlaying();

  // Assert
  const bool expected_should_only_serve_ads_if_media_is_not_playing = true;
  EXPECT_EQ(expected_should_only_serve_ads_if_media_is_not_playing,
            should_only_serve_ads_if_media_is_not_playing);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     DefaultShouldOnlyServeAdsIfMediaIsNotPlayingWhenDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::frequency_capping::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_if_media_is_not_playing =
      features::frequency_capping::ShouldOnlyServeAdsIfMediaIsNotPlaying();

  // Assert
  const bool expected_should_only_serve_ads_if_media_is_not_playing = true;
  EXPECT_EQ(expected_should_only_serve_ads_if_media_is_not_playing,
            should_only_serve_ads_if_media_is_not_playing);
}

TEST(BatAdsFrequencyCappingFeaturesTest, ShouldOnlyServeAdsIfBrowserIsActive) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["should_only_serve_ads_if_browser_is_active"] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_if_browser_is_active =
      features::frequency_capping::ShouldOnlyServeAdsIfBrowserIsActive();

  // Assert
  const bool expected_should_only_serve_ads_if_browser_is_active = false;
  EXPECT_EQ(expected_should_only_serve_ads_if_browser_is_active,
            should_only_serve_ads_if_browser_is_active);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     DefaultShouldOnlyServeAdsIfBrowserIsActive) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool should_only_serve_ads_if_browser_is_active =
      features::frequency_capping::ShouldOnlyServeAdsIfBrowserIsActive();

  // Assert
  const bool expected_should_only_serve_ads_if_browser_is_active = true;
  EXPECT_EQ(expected_should_only_serve_ads_if_browser_is_active,
            should_only_serve_ads_if_browser_is_active);
}

TEST(BatAdsFrequencyCappingFeaturesTest,
     DefaultShouldOnlyServeAdsIfBrowserIsActiveWhenDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::frequency_capping::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_only_serve_ads_if_browser_is_active =
      features::frequency_capping::ShouldOnlyServeAdsIfBrowserIsActive();

  // Assert
  const bool expected_should_only_serve_ads_if_browser_is_active = true;
  EXPECT_EQ(expected_should_only_serve_ads_if_browser_is_active,
            should_only_serve_ads_if_browser_is_active);
}

}  // namespace ads
