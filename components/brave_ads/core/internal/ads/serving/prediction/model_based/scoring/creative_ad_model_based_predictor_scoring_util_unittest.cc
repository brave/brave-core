/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/prediction/model_based/scoring/creative_ad_model_based_predictor_scoring_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_segment_input_variable_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingChildIntentSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_child = true;

  // Act
  const double score = ComputeIntentSegmentScore(input_variable);

  // Assert
  EXPECT_DOUBLE_EQ(1.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingParentIntentSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_parent = true;

  // Act
  const double score = ComputeIntentSegmentScore(input_variable);

  // Assert
  EXPECT_DOUBLE_EQ(1.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeNonMatchingIntentSegmentScore) {
  // Arrange
  const CreativeAdPredictorSegmentInputVariableInfo input_variable;

  // Act
  const double score = ComputeIntentSegmentScore(input_variable);

  // Assert
  EXPECT_DOUBLE_EQ(0.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingChildLatentInterestSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_child = true;

  // Act
  const double score = ComputeLatentInterestSegmentScore(input_variable);

  // Assert
  EXPECT_DOUBLE_EQ(1.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingParentLatentInterestSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_parent = true;

  // Act
  const double score = ComputeLatentInterestSegmentScore(input_variable);

  // Assert
  EXPECT_DOUBLE_EQ(1.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeNonMatchingLatentInterestSegmentScore) {
  // Arrange
  const CreativeAdPredictorSegmentInputVariableInfo input_variable;

  // Act
  const double score = ComputeLatentInterestSegmentScore(input_variable);

  // Assert
  EXPECT_DOUBLE_EQ(0.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingChildInterestSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_child = true;

  // Act
  const double score = ComputeInterestSegmentScore(input_variable);

  // Assert
  EXPECT_DOUBLE_EQ(1.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeMatchingParentInterestSegmentScore) {
  // Arrange
  CreativeAdPredictorSegmentInputVariableInfo input_variable;
  input_variable.does_match_parent = true;

  // Act
  const double score = ComputeInterestSegmentScore(input_variable);

  // Assert
  EXPECT_DOUBLE_EQ(1.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeNonMatchingInterestSegmentScore) {
  // Arrange
  const CreativeAdPredictorSegmentInputVariableInfo input_variable;

  // Act
  const double score = ComputeInterestSegmentScore(input_variable);

  // Assert
  EXPECT_DOUBLE_EQ(0.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest, ComputeLastSeenAdScore) {
  // Arrange
  const base::TimeDelta last_seen = base::Hours(7);

  // Act
  const double score = ComputeLastSeenAdScore(last_seen);

  // Assert
  EXPECT_DOUBLE_EQ(0.29166666666666669, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest, ComputeNeverSeenAdScore) {
  // Arrange
  const absl::optional<base::TimeDelta> last_seen;

  // Act
  const double score = ComputeLastSeenAdScore(last_seen);

  // Assert
  EXPECT_DOUBLE_EQ(1.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeLastSeenAdScoreIfExceeds1Day) {
  // Arrange
  const base::TimeDelta last_seen = base::Days(1) + base::Milliseconds(1);

  // Act
  const double score = ComputeLastSeenAdScore(last_seen);

  // Assert
  EXPECT_DOUBLE_EQ(1.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeLastSeenAdvertiserScore) {
  // Arrange
  const base::TimeDelta last_seen = base::Hours(7);

  // Act
  const double score = ComputeLastSeenAdvertiserScore(last_seen);

  // Assert
  EXPECT_DOUBLE_EQ(0.29166666666666669, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeNeverSeenAdvertiserScore) {
  // Arrange
  const absl::optional<base::TimeDelta> last_seen;

  // Act
  const double score = ComputeLastSeenAdvertiserScore(last_seen);

  // Assert
  EXPECT_DOUBLE_EQ(1.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest,
     ComputeLastSeenAdvertiserScoreIfExceeds1Day) {
  // Arrange
  const base::TimeDelta last_seen = base::Days(1) + base::Milliseconds(1);

  // Act
  const double score = ComputeLastSeenAdvertiserScore(last_seen);

  // Assert
  EXPECT_DOUBLE_EQ(1.0, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest, ComputePriorityScore) {
  // Arrange
  const int priority = 5;

  // Act
  const double score = ComputePriorityScore(priority);

  // Assert
  EXPECT_DOUBLE_EQ(0.2, score);
}

TEST(BraveAdsCreativeAdPredictorScoringUtilTest, ComputeZeroPriorityScore) {
  // Arrange
  const int priority = 0;

  // Act
  const double score = ComputePriorityScore(priority);

  // Assert
  EXPECT_DOUBLE_EQ(0.0, score);
}

}  // namespace brave_ads
