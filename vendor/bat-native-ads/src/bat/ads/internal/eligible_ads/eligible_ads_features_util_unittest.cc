/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/eligible_ads_features_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsEligibleAdsFeaturesUtilTest,
     ToAdPredictorWeightsForEmptyParamValue) {
  // Arrange

  // Act
  const AdPredictorWeights weights = ToAdPredictorWeights("");

  // Assert
  const AdPredictorWeights expected_weights = {};
  EXPECT_EQ(expected_weights, weights);
}

TEST(BatAdsEligibleAdsFeaturesUtilTest,
     ToAdPredictorWeightsForNonNumericParamValue) {
  // Arrange

  // Act
  const AdPredictorWeights weights = ToAdPredictorWeights("1.0, foobar, 2.2");

  // Assert
  const AdPredictorWeights expected_weights = {};
  EXPECT_EQ(expected_weights, weights);
}

TEST(BatAdsEligibleAdsFeaturesUtilTest,
     ToAdPredictorWeightsForAllZeroParamValue) {
  // Arrange

  // Act
  const AdPredictorWeights weights = ToAdPredictorWeights("0.0, 0.0, 0.0");

  // Assert
  const AdPredictorWeights expected_weights = {};
  EXPECT_EQ(expected_weights, weights);
}

TEST(BatAdsEligibleAdsFeaturesUtilTest,
     ToAdPredictorWeightsForSomeZeroParamValue) {
  // Arrange

  // Act
  const AdPredictorWeights weights = ToAdPredictorWeights("0.0, 0.1, 0.0");

  // Assert
  const AdPredictorWeights expected_weights = {0.0, 0.1, 0.0};
  EXPECT_EQ(expected_weights, weights);
}

TEST(BatAdsEligibleAdsFeaturesUtilTest,
     ToAdPredictorWeightsForNegativeParamValue) {
  // Arrange

  // Act
  const AdPredictorWeights weights = ToAdPredictorWeights("1.0, 3.0, -2.0");

  // Assert
  const AdPredictorWeights expected_weights = {};
  EXPECT_EQ(expected_weights, weights);
}

TEST(BatAdsEligibleAdsFeaturesUtilTest,
     ToAdPredictorWeightsForSingleParamValue) {
  // Arrange

  // Act
  const AdPredictorWeights weights = ToAdPredictorWeights("1.0");

  // Assert
  const AdPredictorWeights expected_weights = {1.0};
  EXPECT_EQ(expected_weights, weights);
}

TEST(BatAdsEligibleAdsFeaturesUtilTest, ToAdPredictorWeightsForParamValue) {
  // Arrange

  // Act
  const AdPredictorWeights weights = ToAdPredictorWeights("1.1, 3.3, 2.2");

  // Assert
  const AdPredictorWeights expected_weights = {1.1, 3.3, 2.2};
  EXPECT_EQ(expected_weights, weights);
}

TEST(BatAdsEligibleAdsFeaturesUtilTest,
     ToAdPredictorWeightsForParamValueWithMixedTypes) {
  // Arrange

  // Act
  const AdPredictorWeights weights = ToAdPredictorWeights("1, 3, 2.2");

  // Assert
  const AdPredictorWeights expected_weights = {1.0, 3.0, 2.2};
  EXPECT_EQ(expected_weights, weights);
}

}  // namespace ads
