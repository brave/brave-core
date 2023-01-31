/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_util.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_features.h"
#include "brave/components/brave_ads/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsIdleDetectionUtilTest : public UnitTestBase {};

TEST_F(BatAdsIdleDetectionUtilTest, WasLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_was_locked"] = "true";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool screen_was_locked =
      MaybeScreenWasLocked(/*screen_was_locked*/ true);

  // Assert
  EXPECT_TRUE(screen_was_locked);
}

TEST_F(BatAdsIdleDetectionUtilTest, WasLockedIfShouldDetectScreenWasLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_was_locked"] = "true";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool screen_was_locked =
      MaybeScreenWasLocked(/*screen_was_locked*/ true);

  // Assert
  EXPECT_TRUE(screen_was_locked);
}

TEST_F(BatAdsIdleDetectionUtilTest, WasNotLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_was_locked"] = "true";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool screen_was_locked =
      MaybeScreenWasLocked(/*screen_was_locked*/ false);

  // Assert
  EXPECT_FALSE(screen_was_locked);
}

TEST_F(BatAdsIdleDetectionUtilTest, WasNotLockedIfShouldNotDetectWasLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_was_locked"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool screen_was_locked =
      MaybeScreenWasLocked(/*screen_was_locked*/ true);

  // Assert
  EXPECT_FALSE(screen_was_locked);
}

TEST_F(BatAdsIdleDetectionUtilTest, HasNotExceededMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "10s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool has_exceeded_maximum_idle_time =
      HasExceededMaximumIdleTime(base::Seconds(10));

  // Assert
  EXPECT_FALSE(has_exceeded_maximum_idle_time);
}

TEST_F(BatAdsIdleDetectionUtilTest, HasNotExceededInfiniteMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "0s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool has_exceeded_maximum_idle_time =
      HasExceededMaximumIdleTime(base::TimeDelta::Max());

  // Assert
  EXPECT_FALSE(has_exceeded_maximum_idle_time);
}

TEST_F(BatAdsIdleDetectionUtilTest, HasExceededMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "10s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool has_exceeded_maximum_idle_time =
      HasExceededMaximumIdleTime(base::Seconds(11));

  // Assert
  EXPECT_TRUE(has_exceeded_maximum_idle_time);
}

TEST_F(BatAdsIdleDetectionUtilTest, UpdateIdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams params;
  params["idle_time_threshold"] = "5s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

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

TEST_F(BatAdsIdleDetectionUtilTest, DoNotUpdateIdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams params;
  params["idle_time_threshold"] = "10s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(user_activity::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

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
