/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/eligible_ads_features.h"

#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::features {

namespace {
constexpr unsigned int kNumberOfServingFeatures = 7U;
}  // namespace

TEST(BatAdsEligibleAdsFeaturesTest, EligibleAdsEnabled) {
  // Arrange

  // Act
  const bool is_enabled = IsEligibleAdsEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsEligibleAdsFeaturesTest, AdFeatureWeightLength) {
  // Arrange

  // Act
  const AdPredictorWeightList weights = GetAdPredictorWeights();

  // Assert
  EXPECT_EQ(kNumberOfServingFeatures, weights.size());
}

TEST(BatAdsEligibleAdsFeaturesTest, DefaultAdFeatureWeights) {
  // Arrange

  // Act
  const AdPredictorWeightList weights = GetAdPredictorWeights();

  // Assert
  const AdPredictorWeightList expected_weights = {1.0, 1.0, 1.0, 1.0,
                                                  1.0, 1.0, 1.0};
  EXPECT_EQ(expected_weights, weights);
}

}  // namespace ads::features
