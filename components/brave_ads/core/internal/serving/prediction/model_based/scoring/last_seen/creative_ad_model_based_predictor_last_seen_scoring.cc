/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/last_seen/creative_ad_model_based_predictor_last_seen_scoring.h"

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/last_seen/creative_ad_model_based_predictor_last_seen_input_variable_info.h"

namespace brave_ads {

double ComputeLastSeenScore(
    const CreativeAdModelBasedPredictorLastSeenInputVariableInfo&
        last_seen_input_variable) {
  if (!last_seen_input_variable.value ||
      last_seen_input_variable.value > base::Days(1)) {
    // If the ad was never seen or was seen more than a day ago, return the
    // weight of the input variable. This implies that the ad's score is not
    // influenced by the time since it was last seen.
    return last_seen_input_variable.weight;
  }

  // If the ad was seen less than a day ago, return the weight of the input
  // variable multiplied by the fraction of the day since the ad was last seen.
  // This implies that the more recently the ad was seen, the lower its score
  // will be.
  return last_seen_input_variable.weight *
         (last_seen_input_variable.value->InHours() /
          static_cast<double>(base::Time::kHoursPerDay));
}

}  // namespace brave_ads
