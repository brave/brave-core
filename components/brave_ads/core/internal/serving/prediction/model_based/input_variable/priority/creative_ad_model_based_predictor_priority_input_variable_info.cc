/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <tuple>

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/priority/creative_ad_model_based_predictor_priority_input_variable_info.h"

namespace brave_ads {

bool operator==(
    const CreativeAdModelBasedPredictorPriorityInputVariableInfo& lhs,
    const CreativeAdModelBasedPredictorPriorityInputVariableInfo& rhs) {
  const auto tie =
      [](const CreativeAdModelBasedPredictorPriorityInputVariableInfo&
             input_variable) {
        return std::tie(input_variable.value, input_variable.weight);
      };

  return tie(lhs) == tie(rhs);
}

bool operator!=(
    const CreativeAdModelBasedPredictorPriorityInputVariableInfo& lhs,
    const CreativeAdModelBasedPredictorPriorityInputVariableInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
