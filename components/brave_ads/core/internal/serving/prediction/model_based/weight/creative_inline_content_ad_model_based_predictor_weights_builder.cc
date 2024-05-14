/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_inline_content_ad_model_based_predictor_weights_builder.h"

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_inline_content_ad_model_based_predictor_feature.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_ad_model_based_predictor_weights_info.h"

namespace brave_ads {

CreativeAdModelBasedPredictorWeightsInfo
BuildCreativeInlineContentAdModelBasedPredictorWeights() {
  CreativeAdModelBasedPredictorWeightsInfo weights;

  weights.intent_segment.child =
      kInlineContentAdChildIntentSegmentPredictorWeight.Get();
  weights.intent_segment.parent =
      kInlineContentAdParentIntentSegmentPredictorWeight.Get();

  weights.latent_interest_segment.child =
      kInlineContentAdChildLatentInterestSegmentPredictorWeight.Get();
  weights.latent_interest_segment.parent =
      kInlineContentAdParentLatentInterestSegmentPredictorWeight.Get();

  weights.interest_segment.child =
      kInlineContentAdChildInterestSegmentPredictorWeight.Get();
  weights.interest_segment.parent =
      kInlineContentAdParentInterestSegmentPredictorWeight.Get();

  weights.untargeted_segment =
      kInlineContentAdUntargetedSegmentPredictorWeight.Get();

  weights.last_seen_ad = kInlineContentAdLastSeenPredictorWeight.Get();

  return weights;
}

}  // namespace brave_ads
