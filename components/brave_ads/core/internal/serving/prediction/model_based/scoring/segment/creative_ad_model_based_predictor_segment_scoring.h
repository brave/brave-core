/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SCORING_SEGMENT_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_SCORING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SCORING_SEGMENT_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_SCORING_H_

namespace brave_ads {

struct CreativeAdModelBasedPredictorSegmentInputVariablesInfo;
struct CreativeAdModelBasedPredictorUntargetedSegmentInputVariableInfo;

// The score computed by these functions is used in the prediction model to
// determine which ad to serve to the user. The higher the score, the more
// likely the ad will be served.

double ComputeSegmentScore(
    const CreativeAdModelBasedPredictorSegmentInputVariablesInfo&
        segment_input_variable);

double ComputeSegmentScore(
    const CreativeAdModelBasedPredictorUntargetedSegmentInputVariableInfo&
        segment_input_variable);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SCORING_SEGMENT_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_SCORING_H_
