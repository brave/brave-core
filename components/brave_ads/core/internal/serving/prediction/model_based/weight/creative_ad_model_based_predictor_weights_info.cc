/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_ad_model_based_predictor_weights_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const CreativeAdModelBasedPredictorWeightsInfo& lhs,
                const CreativeAdModelBasedPredictorWeightsInfo& rhs) {
  const auto tie = [](const CreativeAdModelBasedPredictorWeightsInfo& weights) {
    return std::tie(weights.intent_segment, weights.latent_interest_segment,
                    weights.interest_segment, weights.last_seen_ad,
                    weights.last_seen_advertiser, weights.priority);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CreativeAdModelBasedPredictorWeightsInfo& lhs,
                const CreativeAdModelBasedPredictorWeightsInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
