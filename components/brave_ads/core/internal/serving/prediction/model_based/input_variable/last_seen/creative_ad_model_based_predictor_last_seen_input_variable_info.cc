/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/last_seen/creative_ad_model_based_predictor_last_seen_input_variable_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(
    const CreativeAdModelBasedPredictorLastSeenInputVariableInfo& lhs,
    const CreativeAdModelBasedPredictorLastSeenInputVariableInfo& rhs) {
  const auto tie =
      [](const CreativeAdModelBasedPredictorLastSeenInputVariableInfo&
             input_variable) {
        return std::tie(input_variable.value, input_variable.weight);
      };

  return tie(lhs) == tie(rhs);
}

bool operator!=(
    const CreativeAdModelBasedPredictorLastSeenInputVariableInfo& lhs,
    const CreativeAdModelBasedPredictorLastSeenInputVariableInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
