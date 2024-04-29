/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_INPUT_VARIABLE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_INPUT_VARIABLE_UTIL_H_

#include <optional>
#include <string>

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/allocation/seen_ads_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/last_seen/creative_ad_model_based_predictor_last_seen_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/segment/creative_ad_model_based_predictor_segment_weight_info.h"

namespace brave_ads {

struct CreativeAdModelBasedPredictorSegmentInputVariablesInfo;
struct CreativeAdModelBasedPredictorUntargetedSegmentInputVariableInfo;
struct UserModelInfo;

CreativeAdModelBasedPredictorSegmentInputVariablesInfo
ComputeCreativeAdModelBasedPredictorIntentSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment,
    const CreativeAdModelBasedPredictorSegmentWeightInfo& weights);

CreativeAdModelBasedPredictorSegmentInputVariablesInfo
ComputeCreativeAdModelBasedPredictorLatentInterestSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment,
    const CreativeAdModelBasedPredictorSegmentWeightInfo& weights);

CreativeAdModelBasedPredictorSegmentInputVariablesInfo
ComputeCreativeAdModelBasedPredictorInterestSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment,
    const CreativeAdModelBasedPredictorSegmentWeightInfo& weights);

CreativeAdModelBasedPredictorUntargetedSegmentInputVariableInfo
ComputeCreativeAdModelBasedPredictorUntargetedSegmentInputVariable(
    const std::string& segment,
    double weight);

template <typename T>
CreativeAdModelBasedPredictorLastSeenInputVariableInfo
ComputeCreativeAdModelBasedPredictorLastSeenAdInputVariable(
    const T& creative_ad,
    const AdEventList& ad_events,
    double weight) {
  CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_ad_input_variable;

  const std::optional<base::Time> last_seen_ad_at =
      GetLastSeenAdAt(ad_events, creative_ad.creative_instance_id);
  if (last_seen_ad_at) {
    last_seen_ad_input_variable.value = base::Time::Now() - *last_seen_ad_at;
  }

  last_seen_ad_input_variable.weight = weight;

  return last_seen_ad_input_variable;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_INPUT_VARIABLE_UTIL_H_
