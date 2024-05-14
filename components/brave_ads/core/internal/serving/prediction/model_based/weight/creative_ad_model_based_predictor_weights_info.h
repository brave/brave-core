/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_CREATIVE_AD_MODEL_BASED_PREDICTOR_WEIGHTS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_CREATIVE_AD_MODEL_BASED_PREDICTOR_WEIGHTS_INFO_H_

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/segment/creative_ad_model_based_predictor_segment_weight_info.h"

namespace brave_ads {

struct CreativeAdModelBasedPredictorWeightsInfo final {
  bool operator==(const CreativeAdModelBasedPredictorWeightsInfo&) const =
      default;

  CreativeAdModelBasedPredictorSegmentWeightInfo intent_segment;
  CreativeAdModelBasedPredictorSegmentWeightInfo latent_interest_segment;
  CreativeAdModelBasedPredictorSegmentWeightInfo interest_segment;

  double untargeted_segment = 0.0;

  double last_seen_ad = 0.0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_CREATIVE_AD_MODEL_BASED_PREDICTOR_WEIGHTS_INFO_H_
