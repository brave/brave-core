/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/segment/creative_ad_model_based_predictor_segment_weight_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const CreativeAdModelBasedPredictorSegmentWeightInfo& lhs,
                const CreativeAdModelBasedPredictorSegmentWeightInfo& rhs) {
  const auto tie =
      [](const CreativeAdModelBasedPredictorSegmentWeightInfo& weight) {
        return std::tie(weight.child, weight.parent);
      };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CreativeAdModelBasedPredictorSegmentWeightInfo& lhs,
                const CreativeAdModelBasedPredictorSegmentWeightInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
