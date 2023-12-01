/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/creative_ad_model_based_predictor_scoring_util.h"

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/last_seen/creative_ad_model_based_predictor_last_seen_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/priority/creative_ad_model_based_predictor_priority_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_segment_input_variables_info.h"

namespace brave_ads {

double ComputeSegmentScore(
    const CreativeAdModelBasedPredictorSegmentInputVariablesInfo&
        segment_input_variable) {
  if (segment_input_variable.child_matches.value) {
    return segment_input_variable.child_matches.weight;
  }

  if (segment_input_variable.parent_matches.value) {
    return segment_input_variable.parent_matches.weight;
  }

  return 0.0;
}

double ComputeLastSeenScore(
    const CreativeAdModelBasedPredictorLastSeenInputVariableInfo&
        last_seen_input_variable) {
  double weight = last_seen_input_variable.weight;

  if (!last_seen_input_variable.value) {
    return weight;
  }

  if (last_seen_input_variable.value <= base::Days(1)) {
    weight *= last_seen_input_variable.value->InHours() /
              static_cast<double>(base::Time::kHoursPerDay);
  }

  return weight;
}

double ComputePriorityScore(
    const CreativeAdModelBasedPredictorPriorityInputVariableInfo&
        priority_input_variable) {
  return priority_input_variable.value == 0
             ? 0.0
             : priority_input_variable.weight / priority_input_variable.value;
}

}  // namespace brave_ads
