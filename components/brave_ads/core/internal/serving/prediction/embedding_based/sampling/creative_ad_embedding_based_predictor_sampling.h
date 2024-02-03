/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_SAMPLING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_SAMPLING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_SAMPLING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_SAMPLING_H_

#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

#include "base/numerics/ranges.h"
#include "base/rand_util.h"

namespace brave_ads {

// Performs a random sampling operation based on the provided probabilities. It
// generates a random number and iterates through the creative ad probabilities,
// accumulating the probabilities. Once the accumulated sum is greater than or
// approximately equal to the random number, the function returns the
// corresponding creative ad. If no ad is sampled during the iteration (which
// can happen if the probabilities do not sum up to 1), the function returns
// `std::nullopt`.

template <typename T>
std::optional<T> MaybeSampleCreativeAd(
    const std::vector<T>& creative_ads,
    const std::vector<double>& creative_ad_probabilities) {
  const double rand = base::RandDouble();

  double sum = 0.0;

  for (size_t i = 0; i < creative_ads.size(); ++i) {
    sum += creative_ad_probabilities.at(i);

    if (sum >= rand || base::IsApproximatelyEqual(
                           rand, sum, std::numeric_limits<double>::epsilon())) {
      return creative_ads.at(i);
    }
  }

  return std::nullopt;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_SAMPLING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_SAMPLING_H_
