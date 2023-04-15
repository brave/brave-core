/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

TEST(BatAdsIdleDetectionFeaturesTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsIdleDetectionEnabled());
}

TEST(BatAdsIdleDetectionFeaturesTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kIdleDetectionFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsIdleDetectionEnabled());
}

TEST(BatAdsIdleDetectionFeaturesTest, GetIdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams params;
  params["idle_time_threshold"] = "7s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(7), kIdleTimeThreshold.Get());
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultIdleTimeThreshold) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(5), kIdleTimeThreshold.Get());
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultIdleTimeThresholdWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kIdleDetectionFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(5), kIdleTimeThreshold.Get());
}

TEST(BatAdsIdleDetectionFeaturesTest, GetMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "30m";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Minutes(30), kMaximumIdleTime.Get());
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultMaximumIdleTime) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(0), kMaximumIdleTime.Get());
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultMaximumIdleTimeWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kIdleDetectionFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(0), kMaximumIdleTime.Get());
}

TEST(BatAdsIdleDetectionFeaturesTest, ShouldDetectScreenWasLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_screen_was_locked"] = "true";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(kShouldDetectScreenWasLocked.Get());
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultShouldDetectScreenWasLocked) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(kShouldDetectScreenWasLocked.Get());
}

TEST(BatAdsIdleDetectionFeaturesTest, ShouldDetectScreenWasLockedWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kIdleDetectionFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(kShouldDetectScreenWasLocked.Get());
}

}  // namespace brave_ads
