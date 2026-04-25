/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsPurchaseIntentFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kPurchaseIntentFeature));
}

TEST(BraveAdsPurchaseIntentFeatureTest, PurchaseIntentResourceVersion) {
  // Act & Assert
  EXPECT_EQ(1, kPurchaseIntentResourceVersion.Get());
}

TEST(BraveAdsPurchaseIntentFeatureTest, PurchaseIntentThreshold) {
  // Act & Assert
  EXPECT_EQ(3, kPurchaseIntentThreshold.Get());
}

TEST(BraveAdsPurchaseIntentFeatureTest, PurchaseIntentTimeWindow) {
  // Act & Assert
  EXPECT_EQ(base::Days(7), kPurchaseIntentTimeWindow.Get());
}

}  // namespace brave_ads
