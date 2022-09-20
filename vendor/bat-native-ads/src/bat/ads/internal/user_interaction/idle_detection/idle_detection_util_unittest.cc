/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_util.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_features.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsIdleTimeTest : public UnitTestBase {};

TEST_F(BatAdsIdleTimeTest, WasLocked) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kShouldDetectScreenWasLockedParameter[] =
      "should_detect_was_locked";
  parameters[kShouldDetectScreenWasLockedParameter] = "true";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, parameters);

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool screen_was_locked =
      MaybeScreenWasLocked(/*screen_was_locked*/ true);

  // Assert
  EXPECT_TRUE(screen_was_locked);
}

TEST_F(BatAdsIdleTimeTest, WasLockedIfShouldDetectScreenWasLocked) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kShouldDetectScreenWasLockedParameter[] =
      "should_detect_was_locked";
  parameters[kShouldDetectScreenWasLockedParameter] = "true";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, parameters);

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool screen_was_locked =
      MaybeScreenWasLocked(/*screen_was_locked*/ true);

  // Assert
  EXPECT_TRUE(screen_was_locked);
}

TEST_F(BatAdsIdleTimeTest, WasNotLocked) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kShouldDetectScreenWasLockedParameter[] =
      "should_detect_was_locked";
  parameters[kShouldDetectScreenWasLockedParameter] = "true";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, parameters);

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool screen_was_locked =
      MaybeScreenWasLocked(/*screen_was_locked*/ false);

  // Assert
  EXPECT_FALSE(screen_was_locked);
}

TEST_F(BatAdsIdleTimeTest, WasNotLockedIfShouldNotDetectWasLocked) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kShouldDetectScreenWasLockedParameter[] =
      "should_detect_was_locked";
  parameters[kShouldDetectScreenWasLockedParameter] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, parameters);

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool screen_was_locked =
      MaybeScreenWasLocked(/*screen_was_locked*/ true);

  // Assert
  EXPECT_FALSE(screen_was_locked);
}

TEST_F(BatAdsIdleTimeTest, HasNotExceededMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kMaximumIdleTimeParameter[] = "maximum_idle_time";
  parameters[kMaximumIdleTimeParameter] = "10s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, parameters);

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool has_exceeded_maximum_idle_time =
      HasExceededMaximumIdleTime(base::Seconds(10));

  // Assert
  EXPECT_FALSE(has_exceeded_maximum_idle_time);
}

TEST_F(BatAdsIdleTimeTest, HasNotExceededInfiniteMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kMaximumIdleTimeParameter[] = "maximum_idle_time";
  parameters[kMaximumIdleTimeParameter] = "0s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, parameters);

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool has_exceeded_maximum_idle_time =
      HasExceededMaximumIdleTime(base::TimeDelta::Max());

  // Assert
  EXPECT_FALSE(has_exceeded_maximum_idle_time);
}

TEST_F(BatAdsIdleTimeTest, HasExceededMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kMaximumIdleTimeParameter[] = "maximum_idle_time";
  parameters[kMaximumIdleTimeParameter] = "10s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, parameters);

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool has_exceeded_maximum_idle_time =
      HasExceededMaximumIdleTime(base::Seconds(11));

  // Assert
  EXPECT_TRUE(has_exceeded_maximum_idle_time);
}

TEST_F(BatAdsIdleTimeTest, UpdateIdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kIdleTimeThresholdParameter[] = "idle_time_threshold";
  parameters[kIdleTimeThresholdParameter] = "5s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, parameters);

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  AdsClientHelper::GetInstance()->SetIntegerPref(prefs::kIdleTimeThreshold, 10);

  ASSERT_TRUE(MaybeUpdateIdleTimeThreshold());

  // Act
  const int idle_time_threshold =
      AdsClientHelper::GetInstance()->GetIntegerPref(prefs::kIdleTimeThreshold);

  // Assert
  const int expected_idle_time_threshold = 5;
  EXPECT_EQ(expected_idle_time_threshold, idle_time_threshold);
}

TEST_F(BatAdsIdleTimeTest, DoNotUpdateIdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kIdleTimeThresholdParameter[] = "idle_time_threshold";
  parameters[kIdleTimeThresholdParameter] = "10s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, parameters);

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  AdsClientHelper::GetInstance()->SetIntegerPref(prefs::kIdleTimeThreshold, 10);

  ASSERT_FALSE(MaybeUpdateIdleTimeThreshold());

  // Act
  const int idle_time_threshold =
      AdsClientHelper::GetInstance()->GetIntegerPref(prefs::kIdleTimeThreshold);

  // Assert
  const int expected_idle_time_threshold = 10;
  EXPECT_EQ(expected_idle_time_threshold, idle_time_threshold);
}

}  // namespace ads
