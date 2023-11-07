/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_INFO_H_

#include <tuple>
#include <vector>

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable_info.h"

namespace brave_ads {

template <typename T>
struct CreativeAdModelBasedPredictorInfo final {
  T creative_ad;
  CreativeAdModelBasedPredictorInputVariableInfo input_variable;
  double score = 0.0;
};

template <typename T>
bool operator==(const CreativeAdModelBasedPredictorInfo<T>& lhs,
                const CreativeAdModelBasedPredictorInfo<T>& rhs) {
  const auto tie =
      [](const CreativeAdModelBasedPredictorInfo<T>& creative_ad_predictor) {
        return std::tie(creative_ad_predictor.creative_ad,
                        creative_ad_predictor.input_variable,
                        creative_ad_predictor.score);
      };

  return tie(lhs) == tie(rhs);
}

template <typename T>
bool operator!=(const CreativeAdModelBasedPredictorInfo<T>& lhs,
                const CreativeAdModelBasedPredictorInfo<T>& rhs) {
  return !(lhs == rhs);
}

template <typename T>
using CreativeAdModelBasedPredictorList =
    std::vector<CreativeAdModelBasedPredictorInfo<T>>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_INFO_H_
