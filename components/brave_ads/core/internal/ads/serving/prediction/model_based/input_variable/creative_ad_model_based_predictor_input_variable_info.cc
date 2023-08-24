/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <tuple>

#include "brave/components/brave_ads/core/internal/ads/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable_info.h"

namespace brave_ads {

bool operator==(const CreativeAdPredictorInputVariableInfo& lhs,
                const CreativeAdPredictorInputVariableInfo& rhs) {
  const auto tie =
      [](const CreativeAdPredictorInputVariableInfo& input_variable) {
        return std::tie(input_variable.intent_segment,
                        input_variable.latent_interest_segment,
                        input_variable.interest_segment,
                        input_variable.last_seen_ad,
                        input_variable.last_seen_advertiser);
      };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CreativeAdPredictorInputVariableInfo& lhs,
                const CreativeAdPredictorInputVariableInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
