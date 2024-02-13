/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_idle_detection/user_idle_detection_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/public/user_attention/user_idle_detection/user_idle_detection_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUserIdleDetectionUtilTest : public UnitTestBase {};

TEST_F(BraveAdsUserIdleDetectionUtilTest, WasLocked) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserIdleDetectionFeature, {{"should_detect_screen_was_locked", "true"}});

  // Act & Assert
  EXPECT_TRUE(MaybeScreenWasLocked(/*screen_was_locked=*/true));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest,
       WasLockedIfShouldDetectScreenWasLocked) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserIdleDetectionFeature, {{"should_detect_screen_was_locked", "true"}});

  // Act & Assert
  EXPECT_TRUE(MaybeScreenWasLocked(/*screen_was_locked=*/true));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest, WasNotLocked) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserIdleDetectionFeature, {{"should_detect_screen_was_locked", "true"}});

  // Act & Assert
  EXPECT_FALSE(MaybeScreenWasLocked(/*screen_was_locked=*/false));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest,
       WasNotLockedIfShouldNotDetectWasLocked) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserIdleDetectionFeature,
      {{"should_detect_screen_was_locked", "false"}});

  // Act & Assert
  EXPECT_FALSE(MaybeScreenWasLocked(/*screen_was_locked=*/true));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest, HasNotExceededMaximumIdleTime) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserIdleDetectionFeature, {{"maximum_idle_time", "10s"}});

  // Act & Assert
  EXPECT_FALSE(HasExceededMaximumIdleTime(base::Seconds(10)));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest,
       HasNotExceededInfiniteMaximumIdleTime) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserIdleDetectionFeature, {{"maximum_idle_time", "0s"}});

  // Act & Assert
  EXPECT_FALSE(HasExceededMaximumIdleTime(base::TimeDelta::Max()));
}

TEST_F(BraveAdsUserIdleDetectionUtilTest, HasExceededMaximumIdleTime) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserIdleDetectionFeature, {{"maximum_idle_time", "10s"}});

  // Act & Assert
  EXPECT_TRUE(HasExceededMaximumIdleTime(base::Seconds(11)));
}

}  // namespace brave_ads
