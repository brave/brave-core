/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/segment/creative_ad_model_based_predictor_segment_weight_unittest_util.h"

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_ad_model_based_predictor_weights_info.h"

namespace brave_ads::test {

CreativeAdModelBasedPredictorWeightsInfo
BuildCreativeAdModelBasedPredictorWeights() {
  CreativeAdModelBasedPredictorWeightsInfo weights;

  weights.intent_segment.child = 1.0;
  weights.intent_segment.parent = 1.0;

  weights.latent_interest_segment.child = 1.0;
  weights.latent_interest_segment.parent = 1.0;

  weights.interest_segment.child = 1.0;
  weights.interest_segment.parent = 1.0;

  weights.untargeted_segment = 0.0001;

  weights.last_seen_ad = 1.0;

  return weights;
}
}  // namespace brave_ads::test
