/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/user_activity/user_activity_features.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsUserActivityFeaturesTest, Enabled) {
  // Arrange

  // Act
  const bool is_enabled = features::user_activity::IsEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsUserActivityFeaturesTest, Disabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::user_activity::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool is_enabled = features::user_activity::IsEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
}

TEST(BatAdsUserActivityFeaturesTest, Triggers) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kTriggersParameter[] = "triggers";
  parameters[kTriggersParameter] = "01=0.5;010203=1.0;0203=0.75";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const std::string triggers = features::user_activity::GetTriggers();

  // Assert
  const std::string expected_triggers = "01=0.5;010203=1.0;0203=0.75";
  EXPECT_EQ(expected_triggers, triggers);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultTriggers) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const std::string triggers = features::user_activity::GetTriggers();

  // Assert
  const std::string expected_triggers = "01=.5;02=.5;08=1;09=1;0D=1;0E=1";
  EXPECT_EQ(expected_triggers, triggers);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultTriggersWhenDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::user_activity::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const std::string triggers = features::user_activity::GetTriggers();

  // Assert
  const std::string expected_triggers = "01=.5;02=.5;08=1;09=1;0D=1;0E=1";
  EXPECT_EQ(expected_triggers, triggers);
}

TEST(BatAdsUserActivityFeaturesTest, TimeWindow) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kTimeWindowParameter[] = "time_window";
  parameters[kTimeWindowParameter] = "2h";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window = features::user_activity::GetTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Hours(2);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultTimeWindow) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window = features::user_activity::GetTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Hours(1);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultTimeWindowWhenDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::user_activity::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window = features::user_activity::GetTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Hours(1);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsUserActivityFeaturesTest, Threshold) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kThresholdParameter[] = "threshold";
  parameters[kThresholdParameter] = "7.0";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const double threshold = features::user_activity::GetThreshold();

  // Assert
  const double expected_threshold = 7.0;
  EXPECT_EQ(expected_threshold, threshold);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultThreshold) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const double threshold = features::user_activity::GetThreshold();

  // Assert
  const double expected_threshold = 2.0;
  EXPECT_EQ(expected_threshold, threshold);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultThresholdWhenDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::user_activity::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const double threshold = features::user_activity::GetThreshold();

  // Assert
  const double expected_threshold = 2.0;
  EXPECT_EQ(expected_threshold, threshold);
}

TEST(BatAdsUserActivityFeaturesTest, IdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kIdleTimeThresholdParameter[] = "idle_time_threshold";
  parameters[kIdleTimeThresholdParameter] = "5s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta idle_time_threshold =
      features::user_activity::GetIdleTimeThreshold();

  // Assert
  const base::TimeDelta expected_idle_time_threshold = base::Seconds(5);
  EXPECT_EQ(expected_idle_time_threshold, idle_time_threshold);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultIdleTimeThreshold) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta idle_time_threshold =
      features::user_activity::GetIdleTimeThreshold();

  // Assert
  const base::TimeDelta expected_idle_time_threshold = base::Seconds(15);
  EXPECT_EQ(expected_idle_time_threshold, idle_time_threshold);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultIdleTimeThresholdWhenDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::user_activity::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta idle_time_threshold =
      features::user_activity::GetIdleTimeThreshold();

  // Assert
  const base::TimeDelta expected_idle_time_threshold = base::Seconds(15);
  EXPECT_EQ(expected_idle_time_threshold, idle_time_threshold);
}

TEST(BatAdsUserActivityFeaturesTest, MaximumIdleTime) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kMaximumIdleTimeParameter[] = "maximum_idle_time";
  parameters[kMaximumIdleTimeParameter] = "30m";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta maximum_idle_time =
      features::user_activity::GetMaximumIdleTime();

  // Assert
  const base::TimeDelta expected_maximum_idle_time = base::Minutes(30);
  EXPECT_EQ(expected_maximum_idle_time, maximum_idle_time);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultMaximumIdleTime) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta maximum_idle_time =
      features::user_activity::GetMaximumIdleTime();

  // Assert
  const base::TimeDelta expected_maximum_idle_time = base::Seconds(0);
  EXPECT_EQ(expected_maximum_idle_time, maximum_idle_time);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultMaximumIdleTimeWhenDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::user_activity::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta maximum_idle_time =
      features::user_activity::GetMaximumIdleTime();

  // Assert
  const base::TimeDelta expected_maximum_idle_time = base::Seconds(0);
  EXPECT_EQ(expected_maximum_idle_time, maximum_idle_time);
}

TEST(BatAdsUserActivityFeaturesTest, ShouldDetectWasLocked) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kShouldDetectWasLockedParameter[] = "should_detect_was_locked";
  parameters[kShouldDetectWasLockedParameter] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_detect_was_locked =
      features::user_activity::ShouldDetectWasLocked();

  // Assert
  const bool expected_should_detect_was_locked = false;
  EXPECT_EQ(expected_should_detect_was_locked, should_detect_was_locked);
}

TEST(BatAdsUserActivityFeaturesTest, DefaultShouldDetectWasLocked) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_detect_was_locked =
      features::user_activity::ShouldDetectWasLocked();

  // Assert
  const bool expected_should_detect_was_locked = false;
  EXPECT_EQ(expected_should_detect_was_locked, should_detect_was_locked);
}

TEST(BatAdsUserActivityFeaturesTest, ShouldDetectWasLockedWhenDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::user_activity::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_detect_was_locked =
      features::user_activity::ShouldDetectWasLocked();

  // Assert
  const bool expected_should_detect_was_locked = false;
  EXPECT_EQ(expected_should_detect_was_locked, should_detect_was_locked);
}

}  // namespace ads
