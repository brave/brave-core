/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_INPUT_VARIABLE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_INPUT_VARIABLE_UTIL_H_

#include <string>

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/allocation/seen_ads_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/allocation/seen_advertisers_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/last_seen/creative_ad_model_based_predictor_last_seen_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/priority/creative_ad_model_based_predictor_priority_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_ad_model_based_predictor_weights_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct CreativeAdModelBasedPredictorSegmentInputVariablesInfo;
struct UserModelInfo;

CreativeAdModelBasedPredictorSegmentInputVariablesInfo
ComputeCreativeAdModelBasedPredictorIntentSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment,
    const CreativeAdModelBasedPredictorWeightsInfo& weights);

CreativeAdModelBasedPredictorSegmentInputVariablesInfo
ComputeCreativeAdModelBasedPredictorLatentInterestSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment,
    const CreativeAdModelBasedPredictorWeightsInfo& weights);

CreativeAdModelBasedPredictorSegmentInputVariablesInfo
ComputeCreativeAdModelBasedPredictorInterestSegmentInputVariable(
    const UserModelInfo& user_model,
    const std::string& segment,
    const CreativeAdModelBasedPredictorWeightsInfo& weights);

template <typename T>
CreativeAdModelBasedPredictorLastSeenInputVariableInfo
ComputeCreativeAdModelBasedPredictorLastSeenAdInputVariable(
    const T& creative_ad,
    const AdEventList& ad_events,
    const CreativeAdModelBasedPredictorWeightsInfo& weights) {
  CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_ad_input_variable;

  const absl::optional<base::Time> last_seen_at =
      GetLastSeenAdAt(ad_events, creative_ad);
  if (last_seen_at) {
    last_seen_ad_input_variable.value = base::Time::Now() - *last_seen_at;
  }

  last_seen_ad_input_variable.weight = weights.last_seen_ad;

  return last_seen_ad_input_variable;
}

template <typename T>
CreativeAdModelBasedPredictorLastSeenInputVariableInfo
ComputeCreativeAdModelBasedPredictorLastSeenAdvertiserInputVariable(
    const T& creative_ad,
    const AdEventList& ad_events,
    const CreativeAdModelBasedPredictorWeightsInfo& weights) {
  CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_advertiser_input_variable;

  const absl::optional<base::Time> last_seen_at =
      GetLastSeenAdvertiserAt(ad_events, creative_ad);
  if (last_seen_at) {
    last_seen_advertiser_input_variable.value =
        base::Time::Now() - *last_seen_at;
  }

  last_seen_advertiser_input_variable.weight = weights.last_seen_advertiser;

  return last_seen_advertiser_input_variable;
}

template <typename T>
CreativeAdModelBasedPredictorPriorityInputVariableInfo
ComputeCreativeAdModelBasedPredictorPriorityInputVariable(
    const T& creative_ad,
    const CreativeAdModelBasedPredictorWeightsInfo& weights) {
  CreativeAdModelBasedPredictorPriorityInputVariableInfo
      priority_input_variable;

  priority_input_variable.value = creative_ad.priority;

  priority_input_variable.weight = weights.priority;

  return priority_input_variable;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_CREATIVE_AD_MODEL_BASED_PREDICTOR_INPUT_VARIABLE_UTIL_H_
