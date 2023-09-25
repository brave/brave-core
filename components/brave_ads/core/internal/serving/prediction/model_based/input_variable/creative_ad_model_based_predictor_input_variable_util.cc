/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable_util.h"

#include "base/containers/contains.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_segment_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_user_model_segments.h"

namespace brave_ads {

CreativeAdPredictorSegmentInputVariableInfo
ComputeCreativeAdPredictorIntentSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment) {
  CreativeAdPredictorSegmentInputVariableInfo intent_segment;

  intent_segment.does_match_child =
      base::Contains(GetTopChildIntentSegments(user_model), segment);

  intent_segment.does_match_parent = base::Contains(
      GetTopParentIntentSegments(user_model), GetParentSegment(segment));

  return intent_segment;
}

CreativeAdPredictorSegmentInputVariableInfo
ComputeCreativeAdPredictorLatentInterestSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment) {
  CreativeAdPredictorSegmentInputVariableInfo latent_interest_segment;

  latent_interest_segment.does_match_child =
      base::Contains(GetTopChildLatentInterestSegments(user_model), segment);

  latent_interest_segment.does_match_parent =
      base::Contains(GetTopParentLatentInterestSegments(user_model),
                     GetParentSegment(segment));

  return latent_interest_segment;
}

CreativeAdPredictorSegmentInputVariableInfo
ComputeCreativeAdPredictorInterestSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment) {
  CreativeAdPredictorSegmentInputVariableInfo interest_segment;

  interest_segment.does_match_child =
      base::Contains(GetTopChildInterestSegments(user_model), segment);

  interest_segment.does_match_parent = base::Contains(
      GetTopParentInterestSegments(user_model), GetParentSegment(segment));

  return interest_segment;
}

}  // namespace brave_ads
