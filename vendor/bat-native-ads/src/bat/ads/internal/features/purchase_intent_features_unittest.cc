/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/purchase_intent_features.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::targeting::features {

TEST(BatAdsPurchaseIntentFeaturesTest, PurchaseIntentEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsPurchaseIntentEnabled());
}

TEST(BatAdsPurchaseIntentFeaturesTest, PurchaseIntentTreshold) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(3U, GetPurchaseIntentThreshold());
}

TEST(BatAdsPurchaseIntentFeaturesTest, PurchaseIntentTimeWindowInSeconds) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Days(7), GetPurchaseIntentTimeWindow());
}

TEST(BatAdsPurchaseIntentFeaturesTest, PurchaseIntentResource) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(1, GetPurchaseIntentResourceVersion());
}

}  // namespace ads::targeting::features
