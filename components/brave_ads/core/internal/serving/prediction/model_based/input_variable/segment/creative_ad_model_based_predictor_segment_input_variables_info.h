/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_SEGMENT_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_INPUT_VARIABLES_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_SEGMENT_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_INPUT_VARIABLES_INFO_H_

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_segment_input_variable_info.h"

namespace brave_ads {

struct CreativeAdModelBasedPredictorSegmentInputVariablesInfo final {
  bool operator==(const CreativeAdModelBasedPredictorSegmentInputVariablesInfo&)
      const = default;

  CreativeAdModelBasedPredictorSegmentInputVariableInfo child_matches;
  CreativeAdModelBasedPredictorSegmentInputVariableInfo parent_matches;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_SEGMENT_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_INPUT_VARIABLES_INFO_H_
