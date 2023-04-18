/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection_util.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection_features.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsIdleDetectionUtilTest : public UnitTestBase {};

TEST_F(BraveAdsIdleDetectionUtilTest, WasLocked) {
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
  const bool screen_was_locked =
      MaybeScreenWasLocked(/*screen_was_locked*/ true);

  // Assert
  EXPECT_TRUE(screen_was_locked);
}

TEST_F(BraveAdsIdleDetectionUtilTest, WasLockedIfShouldDetectScreenWasLocked) {
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
  const bool screen_was_locked =
      MaybeScreenWasLocked(/*screen_was_locked*/ true);

  // Assert
  EXPECT_TRUE(screen_was_locked);
}

TEST_F(BraveAdsIdleDetectionUtilTest, WasNotLocked) {
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
  const bool screen_was_locked =
      MaybeScreenWasLocked(/*screen_was_locked*/ false);

  // Assert
  EXPECT_FALSE(screen_was_locked);
}

TEST_F(BraveAdsIdleDetectionUtilTest, WasNotLockedIfShouldNotDetectWasLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_screen_was_locked"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetectionFeature, params);

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

TEST_F(BraveAdsIdleDetectionUtilTest, HasNotExceededMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "10s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetectionFeature, params);

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

TEST_F(BraveAdsIdleDetectionUtilTest, HasNotExceededInfiniteMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "0s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetectionFeature, params);

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

TEST_F(BraveAdsIdleDetectionUtilTest, HasExceededMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "10s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetectionFeature, params);

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

TEST_F(BraveAdsIdleDetectionUtilTest, UpdateIdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams params;
  params["idle_time_threshold"] = "5s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  ads_client_mock_->SetIntegerPref(prefs::kIdleTimeThreshold, 10);

  ASSERT_TRUE(MaybeUpdateIdleTimeThreshold());

  // Act
  const int idle_time_threshold =
      ads_client_mock_->GetIntegerPref(prefs::kIdleTimeThreshold);

  // Assert
  EXPECT_EQ(5, idle_time_threshold);
}

TEST_F(BraveAdsIdleDetectionUtilTest, DoNotUpdateIdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams params;
  params["idle_time_threshold"] = "10s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  ads_client_mock_->SetIntegerPref(prefs::kIdleTimeThreshold, 10);

  ASSERT_FALSE(MaybeUpdateIdleTimeThreshold());

  // Act
  const int idle_time_threshold =
      ads_client_mock_->GetIntegerPref(prefs::kIdleTimeThreshold);

  // Assert
  EXPECT_EQ(10, idle_time_threshold);
}

}  // namespace brave_ads
