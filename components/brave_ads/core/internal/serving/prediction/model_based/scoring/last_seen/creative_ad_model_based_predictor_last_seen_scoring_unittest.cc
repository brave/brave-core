/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/last_seen/creative_ad_model_based_predictor_last_seen_scoring.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/last_seen/creative_ad_model_based_predictor_last_seen_input_variable_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeAdModelBasedPredictorLastSeenScoringTest,
     ComputeLastSeenScore) {
  // Arrange
  CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_input_variable;
  last_seen_input_variable.value = base::Hours(7);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, ComputeLastSeenScore(last_seen_input_variable));
}

TEST(BraveAdsCreativeAdModelBasedPredictorLastSeenScoringTest,
     ComputeNeverSeenScore) {
  // Arrange
  CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_input_variable;

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, ComputeLastSeenScore(last_seen_input_variable));
}

TEST(BraveAdsCreativeAdModelBasedPredictorLastSeenScoringTest,
     ComputeLastSeenScoreIfExceeds1Day) {
  // Arrange
  CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_input_variable;
  last_seen_input_variable.value = base::Days(1) + base::Milliseconds(1);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, ComputeLastSeenScore(last_seen_input_variable));
}

}  // namespace brave_ads
