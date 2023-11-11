/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <tuple>

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable_info.h"

namespace brave_ads {

bool operator==(const CreativeAdModelBasedPredictorInputVariableInfo& lhs,
                const CreativeAdModelBasedPredictorInputVariableInfo& rhs) {
  const auto tie =
      [](const CreativeAdModelBasedPredictorInputVariableInfo& input_variable) {
        return std::tie(
            input_variable.intent_segment,
            input_variable.latent_interest_segment,
            input_variable.interest_segment, input_variable.last_seen_ad,
            input_variable.last_seen_advertiser, input_variable.priority);
      };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CreativeAdModelBasedPredictorInputVariableInfo& lhs,
                const CreativeAdModelBasedPredictorInputVariableInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
