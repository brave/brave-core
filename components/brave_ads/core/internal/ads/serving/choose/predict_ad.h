/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/ads/serving/choose/ad_predictor_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/eligible_ads_predictor_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/sample_ads.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pacing/pacing.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/ad_last_seen_hours_ago_predictor_variable_util.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/advertiser_last_seen_hours_ago_predictor_variable_util.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/creative_ad_priority_predictor_variable_util.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/does_match_child_intent_segment_predictor_variable_util.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/does_match_child_interest_segment_predictor_variable_util.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/does_match_parent_intent_segment_predictor_variable_util.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/does_match_parent_interest_segment_predictor_variable_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

template <typename T>
absl::optional<T> PredictAd(const UserModelInfo& user_model,
                            const AdEventList& ad_events,
                            const std::vector<T>& creative_ads) {
  DCHECK(!creative_ads.empty());

  const std::vector<T> paced_creative_ads = PaceCreativeAds(creative_ads);

  CreativeAdPredictorMap<T> creative_ad_predictors;
  creative_ad_predictors =
      GroupCreativeAdsByCreativeInstanceId(paced_creative_ads);
  creative_ad_predictors = ComputePredictorFeaturesAndScores(
      creative_ad_predictors, user_model, ad_events);

  for (const auto &[creative_instance_id, ad_predictor] :
       creative_ad_predictors) {

    float value_does_match_intent_parent_segments =
        (ad_predictor.does_match_intent_parent_segments) ? 1 : 0;
    float value_does_match_intent_child_segments =
        (ad_predictor.does_match_intent_child_segments) ? 1 : 0;
    float value_does_match_interest_parent_segments =
        (ad_predictor.does_match_interest_parent_segments) ? 1 : 0;
    float value_does_match_interest_child_segments =
        (ad_predictor.does_match_interest_child_segments) ? 1 : 0;

    BLOG(2, ad_predictor.ad_last_seen_hours_ago);
    BLOG(2, ad_predictor.advertiser_last_seen_hours_ago);
    BLOG(2, ad_predictor.creative_ad.priority);
    BLOG(2, ad_predictor.does_match_intent_parent_segments);
    BLOG(2, ad_predictor.does_match_intent_child_segments);
    BLOG(2, ad_predictor.does_match_interest_parent_segments);
    BLOG(2, ad_predictor.does_match_interest_child_segments);

    SetAdLastSeenHoursAgoPredictorVariable(ad_predictor.ad_last_seen_hours_ago);
    SetAdvertiserLastSeenHoursAgoPredictorVariable(
        ad_predictor.advertiser_last_seen_hours_ago);
    SetCreativeAdPriorityPredictorVariable(ad_predictor.creative_ad.priority);
    SetDoesMatchParentIntentSegmentPredictorVariable(
        value_does_match_intent_parent_segments);
    SetDoesMatchChildIntentSegmentPredictorVariable(
        value_does_match_intent_child_segments);
    SetDoesMatchParentInterestSegmentPredictorVariable(
        value_does_match_interest_parent_segments);
    SetDoesMatchChildInterestSegmentPredictorVariable(
        value_does_match_interest_child_segments);
  }

  return SampleAdFromPredictors(creative_ad_predictors);
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_H_
