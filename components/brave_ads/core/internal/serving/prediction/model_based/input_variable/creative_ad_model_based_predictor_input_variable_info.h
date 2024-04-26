/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_INPUT_VARIABLE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_INPUT_VARIABLE_INFO_H_

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/last_seen/creative_ad_model_based_predictor_last_seen_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_segment_input_variables_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_untargeted_segment_input_variable_info.h"

namespace brave_ads {

struct CreativeAdModelBasedPredictorInputVariableInfo final {
  bool operator==(const CreativeAdModelBasedPredictorInputVariableInfo&) const =
      default;

  // Whether the creative ad is targeted to the user's intent.
  CreativeAdModelBasedPredictorSegmentInputVariablesInfo intent_segment;

  // Whether the creative ad is targeted to the user's latent interest.
  CreativeAdModelBasedPredictorSegmentInputVariablesInfo
      latent_interest_segment;

  // Whether the creative ad is targeted to the user's interest.
  CreativeAdModelBasedPredictorSegmentInputVariablesInfo interest_segment;

  // Whether the creative ad is untargeted.
  CreativeAdModelBasedPredictorUntargetedSegmentInputVariableInfo
      untargeted_segment;

  // The last time the user saw this ad.
  CreativeAdModelBasedPredictorLastSeenInputVariableInfo last_seen_ad;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_INPUT_VARIABLE_INFO_H_
