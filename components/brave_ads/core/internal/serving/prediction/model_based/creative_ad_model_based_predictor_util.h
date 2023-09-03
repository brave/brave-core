/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_UTIL_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/creative_ad_model_based_predictor_scoring.h"
#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/ad_event_info.h"

namespace brave_ads {

struct UserModelInfo;

template <typename T>
CreativeAdPredictorList<T> ComputeCreativeAdPredictors(
    const std::vector<T>& creative_ads,
    const UserModelInfo& user_model,
    const AdEventList& ad_events) {
  CreativeAdPredictorList<T> creative_ad_predictors;

  for (const auto& creative_ad : creative_ads) {
    CreativeAdPredictorInfo<T> creative_ad_predictor;

    creative_ad_predictor.creative_ad = creative_ad;
    creative_ad_predictor.input_variable =
        ComputeCreativeAdPredictorInputVariable(creative_ad, user_model,
                                                ad_events);
    creative_ad_predictor.score = ComputeCreativeAdPredictorScore(
        creative_ad, creative_ad_predictor.input_variable);

    creative_ad_predictors.push_back(creative_ad_predictor);
  }

  return creative_ad_predictors;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_UTIL_H_
