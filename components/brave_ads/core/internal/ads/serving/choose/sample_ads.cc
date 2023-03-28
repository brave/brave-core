/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/choose/sample_ads.h"

namespace brave_ads {

std::vector<double> ComputeProbabilities(const std::vector<int>& scores) {
  std::vector<double> probabilities;
  probabilities.reserve(scores.size());

  const double normalizing_constant = CalculateNormalizingConstantBase(scores);

  for (const auto& score : scores) {
    probabilities.push_back(score / normalizing_constant);
  }

  return probabilities;
}  // namespace std::vector

}  // namespace brave_ads
