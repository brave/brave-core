/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/creative_ad_model_based_predictor_scoring_util.h"

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_feature.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_segment_input_variable_info.h"

namespace brave_ads {

double ComputeIntentSegmentScore(
    const CreativeAdPredictorSegmentInputVariableInfo& input_variable) {
  if (input_variable.does_match_child) {
    return kChildIntentSegmentAdPredictorWeight.Get();
  }

  if (input_variable.does_match_parent) {
    return kParentIntentSegmentAdPredictorWeight.Get();
  }

  return 0.0;
}

double ComputeLatentInterestSegmentScore(
    const CreativeAdPredictorSegmentInputVariableInfo& input_variable) {
  if (input_variable.does_match_child) {
    return kChildLatentInterestSegmentAdPredictorWeight.Get();
  }

  if (input_variable.does_match_parent) {
    return kParentLatentInterestSegmentAdPredictorWeight.Get();
  }

  return 0.0;
}

double ComputeInterestSegmentScore(
    const CreativeAdPredictorSegmentInputVariableInfo& input_variable) {
  if (input_variable.does_match_child) {
    return kChildInterestSegmentAdPredictorWeight.Get();
  }

  if (input_variable.does_match_parent) {
    return kParentInterestSegmentAdPredictorWeight.Get();
  }

  return 0.0;
}

double ComputeLastSeenAdScore(
    const absl::optional<base::TimeDelta>& last_seen) {
  double weight = kLastSeenAdPredictorWeight.Get();

  if (!last_seen) {
    return weight;
  }

  if (last_seen <= base::Days(1)) {
    weight *=
        last_seen->InHours() / static_cast<double>(base::Time::kHoursPerDay);
  }

  return weight;
}

double ComputeLastSeenAdvertiserScore(
    const absl::optional<base::TimeDelta>& last_seen) {
  double weight = kLastSeenAdvertiserAdPredictorWeight.Get();

  if (!last_seen) {
    return weight;
  }

  if (last_seen <= base::Days(1)) {
    weight *=
        last_seen->InHours() / static_cast<double>(base::Time::kHoursPerDay);
  }

  return weight;
}

double ComputePriorityScore(const int priority) {
  return priority == 0 ? 0.0 : kPriorityAdPredictorWeight.Get() / priority;
}

}  // namespace brave_ads
