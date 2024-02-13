/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/user_attention/user_idle_detection/user_idle_detection_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsUserIdleDetectionFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kUserIdleDetectionFeature));
}

TEST(BraveAdsUserIdleDetectionFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUserIdleDetectionFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kUserIdleDetectionFeature));
}

TEST(BraveAdsUserIdleDetectionFeatureTest, UserIdleDetectionThreshold) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserIdleDetectionFeature, {{"idle_threshold", "1h"}});

  // Act & Assert
  EXPECT_EQ(base::Hours(1), kUserIdleDetectionThreshold.Get());
}

TEST(BraveAdsUserIdleDetectionFeatureTest, DefaultUserIdleDetectionThreshold) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(5), kUserIdleDetectionThreshold.Get());
}

TEST(BraveAdsUserIdleDetectionFeatureTest,
     DefaultUserIdleDetectionThresholdWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUserIdleDetectionFeature);

  // Act & Assert
  EXPECT_EQ(base::Seconds(5), kUserIdleDetectionThreshold.Get());
}

TEST(BraveAdsUserIdleDetectionFeatureTest, MaximumUserIdleDetectionTime) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserIdleDetectionFeature, {{"maximum_idle_time", "30m"}});

  // Act & Assert
  EXPECT_EQ(base::Minutes(30), kMaximumUserIdleDetectionTime.Get());
}

TEST(BraveAdsUserIdleDetectionFeatureTest,
     DefaultMaximumUserIdleDetectionTime) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(0), kMaximumUserIdleDetectionTime.Get());
}

TEST(BraveAdsUserIdleDetectionFeatureTest,
     DefaultMaximumUserIdleDetectionTimeWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUserIdleDetectionFeature);

  // Act & Assert
  EXPECT_EQ(base::Seconds(0), kMaximumUserIdleDetectionTime.Get());
}

TEST(BraveAdsUserIdleDetectionFeatureTest, ShouldDetectScreenWasLocked) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserIdleDetectionFeature, {{"should_detect_screen_was_locked", "true"}});

  // Act & Assert
  EXPECT_TRUE(kShouldDetectScreenWasLocked.Get());
}

TEST(BraveAdsUserIdleDetectionFeatureTest, DefaultShouldDetectScreenWasLocked) {
  // Act & Assert
  EXPECT_FALSE(kShouldDetectScreenWasLocked.Get());
}

TEST(BraveAdsUserIdleDetectionFeatureTest,
     ShouldDetectScreenWasLockedWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUserIdleDetectionFeature);

  // Act & Assert
  EXPECT_FALSE(kShouldDetectScreenWasLocked.Get());
}

}  // namespace brave_ads
