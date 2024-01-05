/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/priority/creative_ad_model_based_predictor_priority_scoring.h"

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/priority/creative_ad_model_based_predictor_priority_input_variable_info.h"

namespace brave_ads {

double ComputePriorityScore(
    const CreativeAdModelBasedPredictorPriorityInputVariableInfo&
        priority_input_variable) {
  if (priority_input_variable.value == 0) {
    return 0.0;
  }

  const double value_reciprocal = 1.0 / priority_input_variable.value;
  return priority_input_variable.weight * value_reciprocal;
}

}  // namespace brave_ads
