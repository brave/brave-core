/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_SAMPLE_ADS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_SAMPLE_ADS_H_

#include <limits>
#include <numeric>
#include <ostream>
#include <vector>

#include "base/notreached.h"
#include "base/numerics/ranges.h"
#include "base/rand_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/ad_predictor_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

template <typename T>
T CalculateNormalizingConstant(const std::vector<T>& scores) {
  return std::accumulate(scores.cbegin(), scores.cend(), T{});
}

template <typename T>
double CalculateNormalizingConstantFromPredictors(
    const CreativeAdPredictorMap<T>& creative_ad_predictors) {
  std::vector<double> scores;
  scores.reserve(creative_ad_predictors.size());

  for (const auto& [segment, ad_predictor] : creative_ad_predictors) {
    scores.push_back(ad_predictor.score);
  }

  return CalculateNormalizingConstant(scores);
}

template <typename T>
absl::optional<T> SampleAdFromPredictors(
    const CreativeAdPredictorMap<T>& creative_ad_predictors) {
  const double normalizing_constant =
      CalculateNormalizingConstantFromPredictors(creative_ad_predictors);
  if (normalizing_constant <= 0.0 ||
      base::IsApproximatelyEqual(normalizing_constant, 0.0,
                                 std::numeric_limits<double>::epsilon())) {
    return absl::nullopt;
  }

  const double rand = base::RandDouble();
  double sum = 0;

  for (const auto& [segment, ad_predictor] : creative_ad_predictors) {
    const double probability = ad_predictor.score / normalizing_constant;
    sum += probability;

    if (rand < sum && !base::IsApproximatelyEqual(
                          rand, sum, std::numeric_limits<double>::epsilon())) {
      return ad_predictor.creative_ad;
    }
  }

  NOTREACHED_NORETURN() << "Sum should always be less than probability";
}

template <typename T>
std::vector<double> ComputeProbabilities(const std::vector<T>& scores) {
  std::vector<double> probabilities;
  probabilities.reserve(scores.size());

  const double normalizing_constant = CalculateNormalizingConstant(scores);

  for (const auto score : scores) {
    probabilities.push_back(score / normalizing_constant);
  }

  return probabilities;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_SAMPLE_ADS_H_
