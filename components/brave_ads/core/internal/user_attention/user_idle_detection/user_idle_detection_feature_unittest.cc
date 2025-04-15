/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/user_attention/user_idle_detection/user_idle_detection_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsUserIdleDetectionFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kUserIdleDetectionFeature));
}

TEST(BraveAdsUserIdleDetectionFeatureTest, UserIdleDetectionThreshold) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(5), kUserIdleDetectionThreshold.Get());
}

TEST(BraveAdsUserIdleDetectionFeatureTest, MaximumUserIdleDetectionTime) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(0), kMaximumUserIdleDetectionTime.Get());
}

TEST(BraveAdsUserIdleDetectionFeatureTest, ShouldDetectScreenWasLocked) {
  // Act & Assert
  EXPECT_FALSE(kShouldDetectScreenWasLocked.Get());
}

}  // namespace brave_ads
