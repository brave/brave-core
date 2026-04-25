/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SAMPLING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SAMPLING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SAMPLING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SAMPLING_H_

#include <limits>
#include <optional>
#include <ostream>  // IWYU pragma: keep

#include "base/notreached.h"
#include "base/numerics/ranges.h"
#include "base/rand_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/sampling/creative_ad_model_based_predictor_sampling_util.h"

namespace brave_ads {

// Uses a sampling algorithm to select a creative ad based on its score. The
// algorithm normalizes the scores of all ads, generates a random number, and
// then iterates through the ads, summing their normalized scores until the sum
// exceeds the random number. The ad where the sum exceeds the random number is
// the one selected.

template <typename T>
std::optional<T> MaybeSampleCreativeAd(
    const CreativeAdModelBasedPredictorList<T>& creative_ad_predictors) {
  if (creative_ad_predictors.empty()) {
    return std::nullopt;
  }

  const double normalizing_constant =
      CalculateNormalizingConstantForCreativeAdModelBasedPredictors(
          creative_ad_predictors);

  if (normalizing_constant <= 0.0 ||
      base::IsApproximatelyEqual(normalizing_constant, 0.0,
                                 std::numeric_limits<double>::epsilon())) {
    return std::nullopt;
  }

  const double rand = base::RandDouble();

  double sum = 0.0;

  for (const auto& creative_ad_predictor : creative_ad_predictors) {
    const double probability =
        creative_ad_predictor.score / normalizing_constant;

    sum += probability;

    if (sum >= rand || base::IsApproximatelyEqual(
                           rand, sum, std::numeric_limits<double>::epsilon())) {
      return creative_ad_predictor.creative_ad;
    }
  }

  NOTREACHED() << "Sum should always be less than probability";
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_SAMPLING_CREATIVE_AD_MODEL_BASED_PREDICTOR_SAMPLING_H_
