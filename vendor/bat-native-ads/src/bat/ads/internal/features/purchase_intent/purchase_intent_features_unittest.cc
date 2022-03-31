/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/purchase_intent/purchase_intent_features.h"

#include <cstdint>

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsPurchaseIntentFeaturesTest, PurchaseIntentEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(features::IsPurchaseIntentEnabled());
}

TEST(BatAdsPurchaseIntentFeaturesTest, PurchaseIntentTreshold) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(3u, features::GetPurchaseIntentThreshold());
}

TEST(BatAdsPurchaseIntentFeaturesTest, PurchaseIntentTimeWindowInSeconds) {
  // Arrange

  // Act

  // Assert
  const int64_t expected_time_window = 7 * (24 * base::Time::kSecondsPerHour);
  EXPECT_EQ(expected_time_window,
            features::GetPurchaseIntentTimeWindowInSeconds());
}

TEST(BatAdsPurchaseIntentFeaturesTest, PurchaseIntentResource) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(1, features::GetPurchaseIntentResourceVersion());
}

}  // namespace ads
