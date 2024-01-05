/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/segment/creative_ad_model_based_predictor_segment_scoring.h"

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

}  // namespace brave_ads
