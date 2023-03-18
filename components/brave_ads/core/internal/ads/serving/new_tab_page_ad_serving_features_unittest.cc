/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::new_tab_page_ads::features {

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

TEST(BatAdsServingFeaturesTest, MaximumAdsPerHour) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["maximum_ads_per_hour"] = "42";
  enabled_features.emplace_back(kServing, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_ads_per_hour = GetMaximumAdsPerHour();

  // Assert
  const int expected_maximum_ads_per_hour = 42;
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
  const int expected_maximum_ads_per_hour = 4;
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
  const int expected_maximum_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_ads_per_hour, maximum_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, MinimumWaitTime) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["minimum_wait_time"] = "10m";
  enabled_features.emplace_back(kServing, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta minimum_wait_time = GetMinimumWaitTime();

  // Assert
  const base::TimeDelta expected_minimum_wait_time = base::Minutes(10);
  EXPECT_EQ(expected_minimum_wait_time, minimum_wait_time);
}

TEST(BatAdsServingFeaturesTest, DefaultMinimumWaitTime) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const base::TimeDelta minimum_wait_time = GetMinimumWaitTime();

  // Assert
  const base::TimeDelta expected_minimum_wait_time = base::Minutes(5);
  EXPECT_EQ(expected_minimum_wait_time, minimum_wait_time);
}

TEST(BatAdsServingFeaturesTest, DisabledMinimumWaitTime) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta minimum_wait_time = GetMinimumWaitTime();

  // Assert
  const base::TimeDelta expected_minimum_wait_time = base::Minutes(5);
  EXPECT_EQ(expected_minimum_wait_time, minimum_wait_time);
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

}  // namespace brave_ads::new_tab_page_ads::features
