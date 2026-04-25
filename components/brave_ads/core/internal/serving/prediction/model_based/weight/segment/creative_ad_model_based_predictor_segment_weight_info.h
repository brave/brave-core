/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_SEGMENT_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_WEIGHT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_SEGMENT_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_WEIGHT_INFO_H_

namespace brave_ads {

struct CreativeAdModelBasedPredictorSegmentWeightInfo final {
  bool operator==(const CreativeAdModelBasedPredictorSegmentWeightInfo&) const =
      default;

  double child = 0.0;
  double parent = 0.0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_SEGMENT_CREATIVE_AD_MODEL_BASED_PREDICTOR_SEGMENT_WEIGHT_INFO_H_
