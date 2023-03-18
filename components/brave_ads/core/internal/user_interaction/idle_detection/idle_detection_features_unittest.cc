/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_interaction/idle_detection/idle_detection_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::idle_detection::features {

TEST(BatAdsIdleDetectionFeaturesTest, IsEnabled) {
  // Arrange

  // Act
  const bool is_enabled = IsEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
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
  const bool is_enabled = IsEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
}

TEST(BatAdsIdleDetectionFeaturesTest, IdleTimeThreshold) {
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
  const base::TimeDelta idle_time_threshold = GetIdleTimeThreshold();

  // Assert
  const base::TimeDelta expected_idle_time_threshold = base::Seconds(7);
  EXPECT_EQ(expected_idle_time_threshold, idle_time_threshold);
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultIdleTimeThreshold) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta idle_time_threshold = GetIdleTimeThreshold();

  // Assert
  const base::TimeDelta expected_idle_time_threshold = base::Seconds(5);
  EXPECT_EQ(expected_idle_time_threshold, idle_time_threshold);
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
  const base::TimeDelta idle_time_threshold = GetIdleTimeThreshold();

  // Assert
  const base::TimeDelta expected_idle_time_threshold = base::Seconds(5);
  EXPECT_EQ(expected_idle_time_threshold, idle_time_threshold);
}

TEST(BatAdsIdleDetectionFeaturesTest, MaximumIdleTime) {
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
  const base::TimeDelta maximum_idle_time = GetMaximumIdleTime();

  // Assert
  const base::TimeDelta expected_maximum_idle_time = base::Minutes(30);
  EXPECT_EQ(expected_maximum_idle_time, maximum_idle_time);
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultMaximumIdleTime) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta maximum_idle_time = GetMaximumIdleTime();

  // Assert
  const base::TimeDelta expected_maximum_idle_time = base::Seconds(0);
  EXPECT_EQ(expected_maximum_idle_time, maximum_idle_time);
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
  const base::TimeDelta maximum_idle_time = GetMaximumIdleTime();

  // Assert
  const base::TimeDelta expected_maximum_idle_time = base::Seconds(0);
  EXPECT_EQ(expected_maximum_idle_time, maximum_idle_time);
}

TEST(BatAdsIdleDetectionFeaturesTest, ShouldDetectScreenWasLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_was_locked"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetection, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_detect_screen_was_locked = ShouldDetectScreenWasLocked();

  // Assert
  const bool expected_should_detect_screen_was_locked = false;
  EXPECT_EQ(expected_should_detect_screen_was_locked,
            should_detect_screen_was_locked);
}

TEST(BatAdsIdleDetectionFeaturesTest, DefaultShouldDetectScreenWasLocked) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_detect_screen_was_locked = ShouldDetectScreenWasLocked();

  // Assert
  const bool expected_should_detect_screen_was_locked = false;
  EXPECT_EQ(expected_should_detect_screen_was_locked,
            should_detect_screen_was_locked);
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
  const bool should_detect_screen_was_locked = ShouldDetectScreenWasLocked();

  // Assert
  const bool expected_should_detect_screen_was_locked = false;
  EXPECT_EQ(expected_should_detect_screen_was_locked,
            should_detect_screen_was_locked);
}

}  // namespace brave_ads::idle_detection::features
