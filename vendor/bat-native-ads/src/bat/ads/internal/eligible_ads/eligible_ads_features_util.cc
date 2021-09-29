/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/eligible_ads_features_util.h"

#include <numeric>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "bat/ads/internal/number_util.h"

namespace ads {

AdPredictorWeights ToAdPredictorWeights(const std::string& param_value) {
  const std::vector<std::string> components = base::SplitString(
      param_value, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  AdPredictorWeights weights;
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

  const double sum = std::accumulate(weights.begin(), weights.end(),
                                     decltype(weights)::value_type(0));
  if (DoubleIsLessEqual(sum, 0)) {
    return {};
  }

  return weights;
}

}  // namespace ads
