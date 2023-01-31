/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/user_activity/user_activity_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::user_activity::features {

TEST(BatAdsUserActivityFeaturesTest, Enabled) {
  // Arrange

  // Act
  const bool is_enabled = IsEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsUserActivityFeaturesTest, Disabled) {
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

TEST(BatAdsUserActivityFeaturesTest, Triggers) {
  // Arrange
  base::FieldTrialParams params;
  params["triggers"] = "01=0.5;010203=1.0;0203=0.75";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const std::string triggers = GetTriggers();

  // Assert
  const std::string expected_triggers = "01=0.5;010203=1.0;0203=0.75";
  EXPECT_EQ(expected_triggers, triggers);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultTriggers) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const std::string triggers = GetTriggers();

  // Assert
  const std::string expected_triggers =
      "0D0B14110D0B14110D0B14110D0B1411=-1.0;0D0B1411070707=-1.0;07070707=-1.0";
  EXPECT_EQ(expected_triggers, triggers);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultTriggersWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const std::string triggers = GetTriggers();

  // Assert
  const std::string expected_triggers =
      "0D0B14110D0B14110D0B14110D0B1411=-1.0;0D0B1411070707=-1.0;07070707=-1.0";
  EXPECT_EQ(expected_triggers, triggers);
}

TEST(BatAdsUserActivityFeaturesTest, TimeWindow) {
  // Arrange
  base::FieldTrialParams params;
  params["time_window"] = "2h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window = GetTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Hours(2);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultTimeWindow) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window = GetTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Minutes(15);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultTimeWindowWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window = GetTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Minutes(15);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsUserActivityFeaturesTest, Threshold) {
  // Arrange
  base::FieldTrialParams params;
  params["threshold"] = "7.0";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const double threshold = GetThreshold();

  // Assert
  const double expected_threshold = 7.0;
  EXPECT_EQ(expected_threshold, threshold);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultThreshold) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const double threshold = GetThreshold();

  // Assert
  const double expected_threshold = 0.0;
  EXPECT_EQ(expected_threshold, threshold);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultThresholdWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const double threshold = GetThreshold();

  // Assert
  const double expected_threshold = 0.0;
  EXPECT_EQ(expected_threshold, threshold);
}

TEST(BatAdsUserActivityFeaturesTest, IdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams params;
  params["idle_time_threshold"] = "7s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

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

TEST(BatAdsUserActivityFeaturesTest, DefaultIdleTimeThreshold) {
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

TEST(BatAdsUserActivityFeaturesTest, DefaultIdleTimeThresholdWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta idle_time_threshold = GetIdleTimeThreshold();

  // Assert
  const base::TimeDelta expected_idle_time_threshold = base::Seconds(5);
  EXPECT_EQ(expected_idle_time_threshold, idle_time_threshold);
}

TEST(BatAdsUserActivityFeaturesTest, MaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "30m";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

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

TEST(BatAdsUserActivityFeaturesTest, DefaultMaximumIdleTime) {
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

TEST(BatAdsUserActivityFeaturesTest, DefaultMaximumIdleTimeWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta maximum_idle_time = GetMaximumIdleTime();

  // Assert
  const base::TimeDelta expected_maximum_idle_time = base::Seconds(0);
  EXPECT_EQ(expected_maximum_idle_time, maximum_idle_time);
}

TEST(BatAdsUserActivityFeaturesTest, ShouldDetectScreenWasLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_was_locked"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

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

TEST(BatAdsUserActivityFeaturesTest, DefaultShouldDetectScreenWasLocked) {
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

TEST(BatAdsUserActivityFeaturesTest, ShouldDetectScreenWasLockedWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

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

}  // namespace ads::user_activity::features
