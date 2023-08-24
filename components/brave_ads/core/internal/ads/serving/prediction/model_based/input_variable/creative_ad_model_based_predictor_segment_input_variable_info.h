/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_INPUT_VARIABLE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_INPUT_VARIABLE_INFO_H_

namespace brave_ads {

struct CreativeAdPredictorSegmentInputVariableInfo final {
  bool does_match_child = false;
  bool does_match_parent = false;
};

bool operator==(const CreativeAdPredictorSegmentInputVariableInfo&,
                const CreativeAdPredictorSegmentInputVariableInfo&);
bool operator!=(const CreativeAdPredictorSegmentInputVariableInfo&,
                const CreativeAdPredictorSegmentInputVariableInfo&);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_INPUT_VARIABLE_INFO_H_
