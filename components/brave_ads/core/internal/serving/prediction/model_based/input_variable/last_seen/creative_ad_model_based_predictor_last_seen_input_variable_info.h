/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_LAST_SEEN_CREATIVE_AD_MODEL_BASED_PREDICTOR_LAST_SEEN_INPUT_VARIABLE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_LAST_SEEN_CREATIVE_AD_MODEL_BASED_PREDICTOR_LAST_SEEN_INPUT_VARIABLE_INFO_H_

#include <optional>

#include "base/time/time.h"

namespace brave_ads {

struct CreativeAdModelBasedPredictorLastSeenInputVariableInfo final {
  bool operator==(const CreativeAdModelBasedPredictorLastSeenInputVariableInfo&)
      const = default;

  // The time delta since the last time the user saw an ad.
  std::optional<base::TimeDelta> value;
  double weight = 0.0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_INPUT_VARIABLE_LAST_SEEN_CREATIVE_AD_MODEL_BASED_PREDICTOR_LAST_SEEN_INPUT_VARIABLE_INFO_H_
