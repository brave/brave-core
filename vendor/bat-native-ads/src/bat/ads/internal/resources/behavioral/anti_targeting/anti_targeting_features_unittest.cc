/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"

#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::resource::features {

TEST(BatAdsAntiTargetingFeaturesTest, IsAntiTargetingEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsAntiTargetingEnabled());
}

TEST(BatAdsAntiTargetingFeaturesTest, GetAntiTargetingResourceVersion) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(1, GetAntiTargetingResourceVersion());
}

}  // namespace ads::resource::features
