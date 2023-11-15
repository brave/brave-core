/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SCORING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SCORING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SCORING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SCORING_UTIL_H_

namespace brave_ads {

struct CreativeAdModelBasedPredictorLastSeenInputVariableInfo;
struct CreativeAdModelBasedPredictorSegmentInputVariablesInfo;
struct CreativeAdModelBasedPredictorPriorityInputVariableInfo;

double ComputeSegmentScore(
    const CreativeAdModelBasedPredictorSegmentInputVariablesInfo&
        segment_input_variable);

double ComputeLastSeenScore(
    const CreativeAdModelBasedPredictorLastSeenInputVariableInfo&
        last_seen_input_variable);

double ComputePriorityScore(
    const CreativeAdModelBasedPredictorPriorityInputVariableInfo&
        priority_input_variable);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SCORING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SCORING_UTIL_H_
