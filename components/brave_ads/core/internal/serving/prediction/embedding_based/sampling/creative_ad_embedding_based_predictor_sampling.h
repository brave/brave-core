/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_SAMPLING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_SAMPLING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_SAMPLING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_SAMPLING_H_

#include <limits>
#include <vector>

#include "base/numerics/ranges.h"
#include "base/rand_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

template <typename T>
absl::optional<T> MaybeSampleCreativeAd(
    const std::vector<T>& creative_ads,
    const std::vector<double>& creative_ad_probabilities) {
  const double rand = base::RandDouble();

  double sum = 0.0;

  for (size_t i = 0; i < creative_ads.size(); ++i) {
    sum += creative_ad_probabilities.at(i);

    if (rand <= sum || base::IsApproximatelyEqual(
                           rand, sum, std::numeric_limits<double>::epsilon())) {
      return creative_ads.at(i);
    }
  }

  return absl::nullopt;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_SAMPLING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_SAMPLING_H_
