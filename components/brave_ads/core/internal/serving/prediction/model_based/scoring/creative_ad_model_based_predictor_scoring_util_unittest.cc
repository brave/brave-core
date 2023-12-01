/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/creative_ad_model_based_predictor_scoring_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/last_seen/creative_ad_model_based_predictor_last_seen_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/priority/creative_ad_model_based_predictor_priority_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_segment_input_variables_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeAdModelBasedPredictorScoringUtilTest,
     ComputeMatchingChildSegmentScore) {
  // Arrange
  CreativeAdModelBasedPredictorSegmentInputVariablesInfo segment_input_variable;
  segment_input_variable.child_matches.value = true;

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeSegmentScore(segment_input_variable));
}

TEST(BraveAdsCreativeAdModelBasedPredictorScoringUtilTest,
     ComputeMatchingParentSegmentScore) {
  // Arrange
  CreativeAdModelBasedPredictorSegmentInputVariablesInfo segment_input_variable;
  segment_input_variable.parent_matches.value = true;

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeSegmentScore(segment_input_variable));
}

TEST(BraveAdsCreativeAdModelBasedPredictorScoringUtilTest,
     ComputeNonMatchingSegmentScore) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, ComputeSegmentScore(/*segment_input_variable=*/{}));
}

TEST(BraveAdsCreativeAdModelBasedPredictorScoringUtilTest,
     ComputeLastSeenScore) {
  // Arrange
  CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_input_variable;
  last_seen_input_variable.value = base::Hours(7);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.29166666666666669,
                   ComputeLastSeenScore(last_seen_input_variable));
}

TEST(BraveAdsCreativeAdModelBasedPredictorScoringUtilTest,
     ComputeNeverSeenScore) {
  // Arrange
  CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_input_variable;

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeLastSeenScore(last_seen_input_variable));
}

TEST(BraveAdsCreativeAdModelBasedPredictorScoringUtilTest,
     ComputeLastSeenScoreIfExceeds1Day) {
  // Arrange
  CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_input_variable;
  last_seen_input_variable.value = base::Days(1) + base::Milliseconds(1);

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeLastSeenScore(last_seen_input_variable));
}

TEST(BraveAdsCreativeAdModelBasedPredictorScoringUtilTest,
     ComputePriorityScore) {
  // Arrange
  CreativeAdModelBasedPredictorPriorityInputVariableInfo
      priority_input_variable;
  priority_input_variable.value = 5;

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.2, ComputePriorityScore(priority_input_variable));
}

TEST(BraveAdsCreativeAdModelBasedPredictorScoringUtilTest,
     ComputeZeroPriorityScore) {
  // Arrange
  CreativeAdModelBasedPredictorPriorityInputVariableInfo
      priority_input_variable;
  priority_input_variable.value = 0;

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, ComputePriorityScore(priority_input_variable));
}

}  // namespace brave_ads
