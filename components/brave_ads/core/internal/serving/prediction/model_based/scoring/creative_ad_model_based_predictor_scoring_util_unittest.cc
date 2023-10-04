/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/creative_ad_model_based_predictor_scoring_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_segment_input_variable_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingChildIntentSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_child = true;

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeIntentSegmentScore(input_variable));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingParentIntentSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_parent = true;

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeIntentSegmentScore(input_variable));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeNonMatchingIntentSegmentScore) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, ComputeIntentSegmentScore(/*input_variable*/ {}));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingChildLatentInterestSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_child = true;

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeLatentInterestSegmentScore(input_variable));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingParentLatentInterestSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_parent = true;

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeLatentInterestSegmentScore(input_variable));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeNonMatchingLatentInterestSegmentScore) {
  // Arrange
  const CreativeAdPredictorSegmentInputVariableInfo input_variable;

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, ComputeLatentInterestSegmentScore(input_variable));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingChildInterestSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_child = true;

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeInterestSegmentScore(input_variable));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingParentInterestSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_parent = true;

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeInterestSegmentScore(input_variable));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeNonMatchingInterestSegmentScore) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, ComputeInterestSegmentScore(/*input_variable*/ {}));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest, ComputeLastSeenAdScore) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.29166666666666669, ComputeLastSeenAdScore(base::Hours(7)));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest, ComputeNeverSeenAdScore) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeLastSeenAdScore({}));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeLastSeenAdScoreIfExceeds1Day) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      1.0, ComputeLastSeenAdScore(base::Days(1) + base::Milliseconds(1)));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeLastSeenAdvertiserScore) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.29166666666666669,
                   ComputeLastSeenAdvertiserScore(base::Hours(7)));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeNeverSeenAdvertiserScore) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeLastSeenAdvertiserScore({}));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeLastSeenAdvertiserScoreIfExceeds1Day) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, ComputeLastSeenAdvertiserScore(base::Days(1) +
                                                       base::Milliseconds(1)));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest, ComputePriorityScore) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.2, ComputePriorityScore(5));
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest, ComputeZeroPriorityScore) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, ComputePriorityScore(0));
}

}  // namespace brave_ads
