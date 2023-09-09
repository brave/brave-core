/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_attention/user_idle_detection/user_idle_detection_util.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/public/user/user_attention/user_idle_detection/user_idle_detection_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUserIdleDetectionUtilTest : public UnitTestBase {};

TEST_F(BraveAdsUserIdleDetectionUtilTest, WasLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_screen_was_locked"] = "true";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kUserIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(MaybeScreenWasLocked(/*screen_was_locked*/ true));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest,
       WasLockedIfShouldDetectScreenWasLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_screen_was_locked"] = "true";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kUserIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(MaybeScreenWasLocked(/*screen_was_locked*/ true));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest, WasNotLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_screen_was_locked"] = "true";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kUserIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(MaybeScreenWasLocked(/*screen_was_locked*/ false));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest,
       WasNotLockedIfShouldNotDetectWasLocked) {
  // Arrange
  base::FieldTrialParams params;
  params["should_detect_screen_was_locked"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kUserIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(MaybeScreenWasLocked(/*screen_was_locked*/ true));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest, HasNotExceededMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "10s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kUserIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(HasExceededMaximumIdleTime(base::Seconds(10)));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest,
       HasNotExceededInfiniteMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "0s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kUserIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(HasExceededMaximumIdleTime(base::TimeDelta::Max()));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest, HasExceededMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams params;
  params["maximum_idle_time"] = "10s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kUserIdleDetectionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(HasExceededMaximumIdleTime(base::Seconds(11)));
}

}  // namespace brave_ads
