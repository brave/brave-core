/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_SAMPLE_ADS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_SAMPLE_ADS_H_

#include <ostream>

#include "base/notreached.h"
#include "base/rand_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/ad_predictor_info.h"
#include "brave/components/brave_ads/core/internal/common/numbers/number_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

template <typename T>
double CalculateNormalizingConstant(
    const CreativeAdPredictorMap<T>& creative_ad_predictors) {
  double normalizing_constant = 0.0;

  for (const auto& [segment, ad_predictor] : creative_ad_predictors) {
    normalizing_constant += ad_predictor.score;
  }

  return normalizing_constant;
}

template <typename T>
absl::optional<T> SampleAdFromPredictors(
    const CreativeAdPredictorMap<T>& creative_ad_predictors) {
  const double normalizing_constant =
      CalculateNormalizingConstant(creative_ad_predictors);
  if (DoubleIsLessEqual(normalizing_constant, 0.0)) {
    return absl::nullopt;
  }

  const double rand = base::RandDouble();
  double sum = 0;

  for (const auto& [segment, ad_predictor] : creative_ad_predictors) {
    const double probability = ad_predictor.score / normalizing_constant;
    sum += probability;

    if (DoubleIsLess(rand, sum)) {
      return ad_predictor.creative_ad;
    }
  }

  NOTREACHED() << "Sum should always be less than probability";
  return absl::nullopt;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_SAMPLE_ADS_H_
