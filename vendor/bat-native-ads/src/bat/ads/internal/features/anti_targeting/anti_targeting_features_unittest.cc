/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/anti_targeting/anti_targeting_features.h"

#include "base/time/time.h"
#include "bat/ads/internal/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsAntiTargetingFeaturesTest, AntiTargetingEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(features::IsAntiTargetingEnabled());
}

TEST(BatAdsAntiTargetingFeaturesTest, AntiTargetingResource) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(1, features::GetAntiTargetingResourceVersion());
}

}  // namespace ads
