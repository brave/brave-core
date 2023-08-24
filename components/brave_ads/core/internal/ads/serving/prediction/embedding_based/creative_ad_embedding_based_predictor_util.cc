/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/prediction/embedding_based/creative_ad_embedding_based_predictor_util.h"

#include <numeric>

namespace brave_ads {

double CalculateNormalizingConstantForVoteRegistry(
    const std::vector<int>& creative_ad_vote_registry) {
  return std::accumulate(creative_ad_vote_registry.cbegin(),
                         creative_ad_vote_registry.cend(), 0.0);
}

std::vector<double> ComputeCreativeAdProbabilitiesForVoteRegistry(
    const std::vector<int>& creative_ad_vote_registry) {
  const double normalizing_constant =
      CalculateNormalizingConstantForVoteRegistry(creative_ad_vote_registry);

  std::vector<double> probabilities;

  for (const int creative_ad_vote : creative_ad_vote_registry) {
    const double probability = creative_ad_vote / normalizing_constant;

    probabilities.push_back(probability);
  }

  return probabilities;
}

}  // namespace brave_ads
