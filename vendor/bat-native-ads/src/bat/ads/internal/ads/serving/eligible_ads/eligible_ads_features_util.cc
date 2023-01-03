/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/eligible_ads_features_util.h"

#include <numeric>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "bat/ads/internal/common/numbers/number_util.h"

namespace ads {

AdPredictorWeightList ToAdPredictorWeights(const std::string& param_value) {
  const std::vector<std::string> components = base::SplitString(
      param_value, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  AdPredictorWeightList weights;
  for (const auto& component : components) {
    double value_as_double;
    if (!base::StringToDouble(component, &value_as_double)) {
      return {};
    }

    if (DoubleIsLess(value_as_double, 0)) {
      return {};
    }

    weights.push_back(value_as_double);
  }

  const double sum =
      std::accumulate(weights.cbegin(), weights.cend(),
                      static_cast<decltype(weights)::value_type>(0));
  if (DoubleIsLessEqual(sum, 0)) {
    return {};
  }

  return weights;
}

}  // namespace ads
