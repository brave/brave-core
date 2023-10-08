/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"

#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsPurchaseIntentFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kPurchaseIntentFeature));
}

TEST(BraveAdsPurchaseIntentFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPurchaseIntentFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kPurchaseIntentFeature));
}

TEST(BraveAdsPurchaseIntentFeatureTest, PurchaseIntentResourceVersion) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPurchaseIntentFeature, {{"resource_version", "0"}});

  // Act & Assert
  EXPECT_EQ(0, kPurchaseIntentResourceVersion.Get());
}

TEST(BraveAdsPurchaseIntentFeatureTest, DefaultPurchaseIntentResourceVersion) {
  // Act & Assert
  EXPECT_EQ(1, kPurchaseIntentResourceVersion.Get());
}

TEST(BraveAdsPurchaseIntentFeatureTest,
     DefaultPurchaseIntentResourceVersionWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPurchaseIntentFeature);

  // Act & Assert
  EXPECT_EQ(1, kPurchaseIntentResourceVersion.Get());
}

TEST(BraveAdsPurchaseIntentFeatureTest, PurchaseIntentThreshold) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(kPurchaseIntentFeature,
                                                         {{"threshold", "5"}});

  // Act & Assert
  EXPECT_EQ(5, kPurchaseIntentThreshold.Get());
}

TEST(BraveAdsPurchaseIntentFeatureTest, DefaultPurchaseIntentThreshold) {
  // Act & Assert
  EXPECT_EQ(3, kPurchaseIntentThreshold.Get());
}

TEST(BraveAdsPurchaseIntentFeatureTest,
     DefaultPurchaseIntentThresholdWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPurchaseIntentFeature);

  // Act & Assert
  EXPECT_EQ(3, kPurchaseIntentThreshold.Get());
}

TEST(BraveAdsPurchaseIntentFeatureTest, PurchaseIntentTimeWindow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPurchaseIntentFeature, {{"time_window", "1d"}});

  // Act & Assert
  EXPECT_EQ(base::Days(1), kPurchaseIntentTimeWindow.Get());
}

TEST(BraveAdsPurchaseIntentFeatureTest, DefaultPurchaseIntentTimeWindow) {
  // Act & Assert
  EXPECT_EQ(base::Days(7), kPurchaseIntentTimeWindow.Get());
}

TEST(BraveAdsPurchaseIntentFeatureTest,
     DefaultPurchaseIntentTimeWindowWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPurchaseIntentFeature);

  // Act & Assert
  EXPECT_EQ(base::Days(7), kPurchaseIntentTimeWindow.Get());
}

}  // namespace brave_ads
