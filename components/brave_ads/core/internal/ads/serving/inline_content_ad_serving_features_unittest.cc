/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::inline_content_ads::features {

TEST(BatAdsServingFeaturesTest, IsServingEnabled) {
  // Arrange

  // Act
  const bool is_enabled = IsServingEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsServingFeaturesTest, IsServingDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool is_enabled = IsServingEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
}

TEST(BatAdsServingFeaturesTest, DefaultServingVersion) {
  // Arrange

  // Act
  const int serving_version = GetServingVersion();

  // Assert
  const int expected_serving_version = 2;
  EXPECT_EQ(expected_serving_version, serving_version);
}

TEST(BatAdsServingFeaturesTest, MaximumAdsPerHour) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["maximum_ads_per_hour"] = "21";
  enabled_features.emplace_back(kServing, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_ads_per_hour = GetMaximumAdsPerHour();

  // Assert
  const int expected_maximum_ads_per_hour = 21;
  EXPECT_EQ(expected_maximum_ads_per_hour, maximum_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DefaultMaximumAdsPerHour) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_ads_per_hour = GetMaximumAdsPerHour();

  // Assert
  const int expected_maximum_ads_per_hour = 6;
  EXPECT_EQ(expected_maximum_ads_per_hour, maximum_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DisabledMaximumAdsPerHour) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_ads_per_hour = GetMaximumAdsPerHour();

  // Assert
  const int expected_maximum_ads_per_hour = 6;
  EXPECT_EQ(expected_maximum_ads_per_hour, maximum_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, MaximumAdsPerDay) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["maximum_ads_per_day"] = "24";
  enabled_features.emplace_back(kServing, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_ads_per_day = GetMaximumAdsPerDay();

  // Assert
  const int expected_maximum_ads_per_day = 24;
  EXPECT_EQ(expected_maximum_ads_per_day, maximum_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DefaultMaximumAdsPerDay) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_ads_per_day = GetMaximumAdsPerDay();

  // Assert
  const int expected_maximum_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_ads_per_day, maximum_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DisabledMaximumAdsPerDay) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_ads_per_day = GetMaximumAdsPerDay();

  // Assert
  const int expected_maximum_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_ads_per_day, maximum_ads_per_day);
}

}  // namespace brave_ads::inline_content_ads::features
