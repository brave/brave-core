/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/eligible_ads_features.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
const unsigned int kNumberOfAdServingFeatures = 7u;
}  // namespace

TEST(BatAdsEligibleAdsFeaturesTest, EligibleAdsEnabled) {
  // Arrange

  // Act
  const bool is_enabled = features::IsEligibleAdsEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsEligibleAdsFeaturesTest, AdFeatureWeightLength) {
  // Arrange

  // Act
  const AdPredictorWeights weights = features::GetAdPredictorWeights();

  // Assert
  EXPECT_EQ(kNumberOfAdServingFeatures, weights.size());
}

TEST(BatAdsEligibleAdsFeaturesTest, DefaultAdFeatureWeights) {
  // Arrange

  // Act
  const AdPredictorWeights weights = features::GetAdPredictorWeights();

  // Assert
  AdPredictorWeights expected_weights = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
  EXPECT_EQ(expected_weights, weights);
}

}  // namespace ads
