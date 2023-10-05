/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/creative_ad_embedding_based_predictor_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeAdEmbeddingBasedPredictorUtilTest,
     CalculateNormalizingConstantForVoteRegistry) {
  // Arrange
  const std::vector<int> creative_ad_vote_registry = {1, 2, 3};

  // Act & Assert
  EXPECT_DOUBLE_EQ(6.0, CalculateNormalizingConstantForVoteRegistry(
                            creative_ad_vote_registry));
}

TEST(BraveAdsCreativeAdEmbeddingBasedPredictorUtilTest,
     CalculateNormalizingConstantForEmptyVoteRegistry) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, CalculateNormalizingConstantForVoteRegistry(
                            /*creative_ad_vote_registry=*/{}));
}

TEST(BraveAdsCreativeAdEmbeddingBasedPredictorUtilTest,
     ComputeCreativeAdProbabilitiesForVoteRegistry) {
  // Arrange
  const std::vector<int> creative_ad_vote_registry = {1, 2, 3};

  // Act
  const std::vector<double> creative_ad_propabilities =
      ComputeCreativeAdProbabilitiesForVoteRegistry(creative_ad_vote_registry);

  // Assert
  EXPECT_DOUBLE_EQ(0.16666666666666666, creative_ad_propabilities.at(0));
  EXPECT_DOUBLE_EQ(0.33333333333333331, creative_ad_propabilities.at(1));
  EXPECT_DOUBLE_EQ(0.5, creative_ad_propabilities.at(2));
}

TEST(BraveAdsCreativeAdEmbeddingBasedPredictorUtilTest,
     ComputeCreativeAdProbabilitiesForEmptyVoteRegistry) {
  // Arrange
  const std::vector<int> creative_ad_vote_registry;

  // Act
  const std::vector<double> creative_ad_propabilities =
      ComputeCreativeAdProbabilitiesForVoteRegistry(creative_ad_vote_registry);

  // Assert
  EXPECT_TRUE(creative_ad_propabilities.empty());
}

}  // namespace brave_ads
