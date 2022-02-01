/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/frequency_capping/anti_targeting/anti_targeting_features.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace resource {
namespace features {

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

}  // namespace features
}  // namespace resource
}  // namespace ads
