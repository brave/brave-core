/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_new_tab_page_ad_model_based_predictor_weights_builder.h"

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_new_tab_page_ad_model_based_predictor_feature.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_ad_model_based_predictor_weights_info.h"

namespace brave_ads {

CreativeAdModelBasedPredictorWeightsInfo
BuildCreativeNewTabPageAdModelBasedPredictorWeights() {
  CreativeAdModelBasedPredictorWeightsInfo weights;

  weights.intent_segment.child =
      kNewTabPageAdChildIntentSegmentPredictorWeight.Get();
  weights.intent_segment.parent =
      kNewTabPageAdParentIntentSegmentPredictorWeight.Get();

  weights.latent_interest_segment.child =
      kNewTabPageAdChildLatentInterestSegmentPredictorWeight.Get();
  weights.latent_interest_segment.parent =
      kNewTabPageAdParentLatentInterestSegmentPredictorWeight.Get();

  weights.interest_segment.child =
      kNewTabPageAdChildInterestSegmentPredictorWeight.Get();
  weights.interest_segment.parent =
      kNewTabPageAdParentInterestSegmentPredictorWeight.Get();

  weights.untargeted_segment =
      kNewTabPageAdUntargetedSegmentPredictorWeight.Get();

  weights.last_seen_ad = kNewTabPageAdLastSeenPredictorWeight.Get();

  return weights;
}

}  // namespace brave_ads
