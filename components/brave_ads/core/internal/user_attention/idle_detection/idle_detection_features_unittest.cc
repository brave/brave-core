/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::idle_detection::features {

TEST(BatAdsIdleDetectionFeaturesTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsEnabled());
}

TEST(BatAdsIdleDetectionFeaturesTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kIdleDetection);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsEnabled());
}

TEST(BatAdsIdleDetectionFeaturesTest, GetIdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams params;
  params["idle_time_threshold"] = "7s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetection, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(7), GetIdleTimeThreshold());
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultIdleTimeThreshold) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(5), GetIdleTimeThreshold());
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultIdleTimeThresholdWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kIdleDetection);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(5), GetIdleTimeThreshold());
}

TEST(BatAdsIdleDetectionFeaturesTest, GetMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "30m";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetection, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Minutes(30), GetMaximumIdleTime());
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultMaximumIdleTime) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(0), GetMaximumIdleTime());
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultMaximumIdleTimeWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kIdleDetection);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(0), GetMaximumIdleTime());
}

TEST(BatAdsIdleDetectionFeaturesTest, ShouldDetectScreenWasLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_screen_was_locked"] = "true";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetection, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(ShouldDetectScreenWasLocked());
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultShouldDetectScreenWasLocked) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ShouldDetectScreenWasLocked());
}

TEST(BatAdsIdleDetectionFeaturesTest, ShouldDetectScreenWasLockedWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kIdleDetection);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(ShouldDetectScreenWasLocked());
}

}  // namespace brave_ads::idle_detection::features
