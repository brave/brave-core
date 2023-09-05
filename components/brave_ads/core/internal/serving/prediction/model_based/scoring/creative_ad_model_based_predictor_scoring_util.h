/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SCORING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SCORING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SCORING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SCORING_UTIL_H_

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

struct CreativeAdPredictorSegmentInputVariableInfo;

double ComputeIntentSegmentScore(
    const CreativeAdPredictorSegmentInputVariableInfo& input_variable);
double ComputeInterestSegmentScore(
    const CreativeAdPredictorSegmentInputVariableInfo& input_variable);
double ComputeLatentInterestSegmentScore(
    const CreativeAdPredictorSegmentInputVariableInfo& input_variable);

double ComputeLastSeenAdScore(const absl::optional<base::TimeDelta>& last_seen);
double ComputeLastSeenAdvertiserScore(
    const absl::optional<base::TimeDelta>& last_seen);

double ComputePriorityScore(int priority);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SCORING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SCORING_UTIL_H_
