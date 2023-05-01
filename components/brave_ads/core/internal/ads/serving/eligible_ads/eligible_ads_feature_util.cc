/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_feature_util.h"

#include <limits>
#include <numeric>
#include <vector>

#include "base/numerics/ranges.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_alias.h"

namespace brave_ads {

AdPredictorWeightList ToAdPredictorWeights(const std::string& param_value) {
  const std::vector<std::string> components = base::SplitString(
      param_value, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  AdPredictorWeightList weights;
  for (const auto& component : components) {
    double weight;
    if (!base::StringToDouble(component, &weight)) {
      return {};
    }

    if (weight < 0.0 &&
        !base::IsApproximatelyEqual(weight, 0.0,
                                    std::numeric_limits<double>::epsilon())) {
      return {};
    }

    weights.push_back(weight);
  }

  const double sum = std::accumulate(weights.cbegin(), weights.cend(), 0.0);
  if (sum <= 0.0 || base::IsApproximatelyEqual(
                        sum, 0.0, std::numeric_limits<double>::epsilon())) {
    return {};
  }

  return weights;
}

}  // namespace brave_ads
