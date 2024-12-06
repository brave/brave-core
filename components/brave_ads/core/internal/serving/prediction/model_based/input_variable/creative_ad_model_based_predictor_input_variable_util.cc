/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable_util.h"

#include "base/containers/contains.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/segments/segment_constants.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_segment_input_variables_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_untargeted_segment_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_user_model_segments.h"

namespace brave_ads {

namespace {

CreativeAdModelBasedPredictorSegmentInputVariablesInfo
ComputeSegmentInputVariable(
    const SegmentList& top_child_segments,
    const SegmentList& top_parent_segments,
    const std::string& segment,
    const CreativeAdModelBasedPredictorSegmentWeightInfo& weights) {
  CreativeAdModelBasedPredictorSegmentInputVariablesInfo segment_input_variable;

  segment_input_variable.child_matches.value =
      base::Contains(top_child_segments, segment);
  segment_input_variable.child_matches.weight = weights.child;

  segment_input_variable.parent_matches.value =
      base::Contains(top_parent_segments, GetParentSegment(segment));
  segment_input_variable.parent_matches.weight = weights.parent;

  return segment_input_variable;
}

}  // namespace

CreativeAdModelBasedPredictorSegmentInputVariablesInfo
ComputeCreativeAdModelBasedPredictorIntentSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment,
    const CreativeAdModelBasedPredictorSegmentWeightInfo& weights) {
  return ComputeSegmentInputVariable(GetTopChildIntentSegments(user_model),
                                     GetTopParentIntentSegments(user_model),
                                     segment, weights);
}

CreativeAdModelBasedPredictorSegmentInputVariablesInfo
ComputeCreativeAdModelBasedPredictorLatentInterestSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment,
    const CreativeAdModelBasedPredictorSegmentWeightInfo& weights) {
  return ComputeSegmentInputVariable(
      GetTopChildLatentInterestSegments(user_model),
      GetTopParentLatentInterestSegments(user_model), segment, weights);
}

CreativeAdModelBasedPredictorSegmentInputVariablesInfo
ComputeCreativeAdModelBasedPredictorInterestSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment,
    const CreativeAdModelBasedPredictorSegmentWeightInfo& weights) {
  return ComputeSegmentInputVariable(GetTopChildInterestSegments(user_model),
                                     GetTopParentInterestSegments(user_model),
                                     segment, weights);
}

CreativeAdModelBasedPredictorUntargetedSegmentInputVariableInfo
ComputeCreativeAdModelBasedPredictorUntargetedSegmentInputVariable(
    const std::string& segment,
    double weight) {
  return CreativeAdModelBasedPredictorUntargetedSegmentInputVariableInfo{
      .value = segment == kUntargetedSegment, .weight = weight};
}

}  // namespace brave_ads
