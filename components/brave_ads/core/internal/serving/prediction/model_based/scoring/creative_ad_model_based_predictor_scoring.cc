/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/creative_ad_model_based_predictor_scoring.h"

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/creative_ad_model_based_predictor_scoring_util.h"

namespace brave_ads {

double ComputeCreativeAdModelBasedPredictorScore(
    const CreativeAdModelBasedPredictorInputVariableInfo& input_variable) {
  double score = 0.0;

  score += ComputeSegmentScore(input_variable.intent_segment);
  score += ComputeSegmentScore(input_variable.latent_interest_segment);
  score += ComputeSegmentScore(input_variable.interest_segment);

  score += ComputeLastSeenScore(input_variable.last_seen_ad);
  score += ComputeLastSeenScore(input_variable.last_seen_advertiser);

  score += ComputePriorityScore(input_variable.priority);

  return score;
}

}  // namespace brave_ads
