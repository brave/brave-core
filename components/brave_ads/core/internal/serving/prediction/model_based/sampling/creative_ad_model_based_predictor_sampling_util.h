/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SAMPLING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SAMPLING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SAMPLING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SAMPLING_UTIL_H_

#include <numeric>

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_info.h"

namespace brave_ads {

template <typename T>
double CalculateNormalizingConstantForCreativeAdModelBasedPredictors(
    const CreativeAdModelBasedPredictorList<T>& creative_ad_predictors) {
  return std::accumulate(
      creative_ad_predictors.cbegin(), creative_ad_predictors.cend(), 0.0,
      [](double normalizing_constant,
         const CreativeAdModelBasedPredictorList<T>::value_type&
             creative_ad_predictor) {
        return normalizing_constant + creative_ad_predictor.score;
      });
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SAMPLING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SAMPLING_UTIL_H_
