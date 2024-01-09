/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/segment/creative_ad_model_based_predictor_segment_scoring.h"

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_segment_input_variables_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_untargeted_segment_input_variable_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeAdModelBasedPredictorSegmentScoringTest,
     ComputeMatchingChildSegmentScore) {
  // Arrange
  CreativeAdModelBasedPredictorSegmentInputVariablesInfo segment_input_variable;
  segment_input_variable.child_matches.value = true;

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeSegmentScore(segment_input_variable));
}

TEST(BraveAdsCreativeAdModelBasedPredictorSegmentScoringTest,
     ComputeMatchingParentSegmentScore) {
  // Arrange
  CreativeAdModelBasedPredictorSegmentInputVariablesInfo segment_input_variable;
  segment_input_variable.parent_matches.value = true;

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeSegmentScore(segment_input_variable));
}

TEST(BraveAdsCreativeAdModelBasedPredictorSegmentScoringTest,
     ComputeNonMatchingSegmentScore) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, ComputeSegmentScore(
               CreativeAdModelBasedPredictorSegmentInputVariablesInfo{}));
}

TEST(BraveAdsCreativeAdModelBasedPredictorSegmentScoringTest,
     ComputeMatchingUntargetedSegmentScore) {
  // Arrange
  CreativeAdModelBasedPredictorUntargetedSegmentInputVariableInfo
      segment_input_variable;
  segment_input_variable.value = true;

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0001, ComputeSegmentScore(segment_input_variable));
}

TEST(BraveAdsCreativeAdModelBasedPredictorSegmentScoringTest,
     ComputeNonMatchingUntargetedSegmentScore) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0,
      ComputeSegmentScore(
          CreativeAdModelBasedPredictorUntargetedSegmentInputVariableInfo{}));
}

}  // namespace brave_ads
