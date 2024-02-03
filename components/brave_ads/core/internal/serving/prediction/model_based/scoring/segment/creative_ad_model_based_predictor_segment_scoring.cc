/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/segment/creative_ad_model_based_predictor_segment_scoring.h"

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_segment_input_variables_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_untargeted_segment_input_variable_info.h"

namespace brave_ads {

namespace {
constexpr double kNoMatchScore = 0.0;
}  // namespace

double ComputeSegmentScore(
    const CreativeAdModelBasedPredictorSegmentInputVariablesInfo&
        segment_input_variable) {
  // Compute the score of a segment based on whether the segment matches a child
  // or parent segment. If there is no match, do not serve the ad.

  if (segment_input_variable.child_matches.value) {
    return segment_input_variable.child_matches.weight;
  }

  if (segment_input_variable.parent_matches.value) {
    return segment_input_variable.parent_matches.weight;
  }

  return kNoMatchScore;
}

double ComputeSegmentScore(
    const CreativeAdModelBasedPredictorUntargetedSegmentInputVariableInfo&
        segment_input_variable) {
  // Compute the score of a segment based on whether the segment matches an
  // untargeted segment. If there is no match, do not serve the ad.
  return segment_input_variable.value ? segment_input_variable.weight
                                      : kNoMatchScore;
}

}  // namespace brave_ads
