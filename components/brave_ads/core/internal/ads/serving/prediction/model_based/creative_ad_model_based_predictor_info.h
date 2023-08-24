/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_INFO_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/ads/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable_info.h"

namespace brave_ads {

template <typename T>
struct CreativeAdPredictorInfo final {
  T creative_ad;
  CreativeAdPredictorInputVariableInfo input_variable;
  double score = 0.0;
};

template <typename T>
bool operator==(const CreativeAdPredictorInfo<T>& lhs,
                const CreativeAdPredictorInfo<T>& rhs) {
  const auto tie = [](const CreativeAdPredictorInfo<T>& creative_ad_predictor) {
    return std::tie(creative_ad_predictor.creative_ad,
                    creative_ad_predictor.input_variable,
                    creative_ad_predictor.score);
  };

  return tie(lhs) == tie(rhs);
}

template <typename T>
bool operator!=(const CreativeAdPredictorInfo<T>& lhs,
                const CreativeAdPredictorInfo<T>& rhs) {
  return !(lhs == rhs);
}

template <typename T>
using CreativeAdPredictorList = std::vector<CreativeAdPredictorInfo<T>>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_INFO_H_
