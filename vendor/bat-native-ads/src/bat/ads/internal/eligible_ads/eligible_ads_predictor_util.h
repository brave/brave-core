/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_PREDICTOR_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_PREDICTOR_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ads/internal/ad_events/ad_event_util.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/eligible_ads/ad_predictor_info.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_aliases.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_features.h"
#include "bat/ads/internal/segments/segments_aliases.h"

namespace ads {

constexpr size_t kDoesMatchIntentChildSegmentsIndex = 0;
constexpr size_t kDoesMatchIntentParentSegmentsIndex = 1;
constexpr size_t kDoesMatchInterestChildSegmentsIndex = 2;
constexpr size_t kDoesMatchInterestParentSegmentsIndex = 3;
constexpr size_t AdLastSeenHoursAgoIndex = 4;
constexpr size_t kAdvertiserLastSeenHoursAgoIndex = 5;
constexpr size_t kPriorityIndex = 6;

template <typename T>
CreativeAdPredictorMap<T> GroupCreativeAdsByCreativeInstanceId(
    const std::vector<T>& creative_ads) {
  CreativeAdPredictorMap<T> creative_ad_predictors;

  for (const auto& creative_ad : creative_ads) {
    const auto iter =
        creative_ad_predictors.find(creative_ad.creative_instance_id);
    if (iter != creative_ad_predictors.end()) {
      iter->second.segments.push_back(creative_ad.segment);
      continue;
    }

    AdPredictorInfo<T> ad_predictor;
    ad_predictor.segments = {creative_ad.segment};
    ad_predictor.creative_ad = creative_ad;

    creative_ad_predictors.insert(
        {creative_ad.creative_instance_id, ad_predictor});
  }

  return creative_ad_predictors;
}

template <typename T>
AdPredictorInfo<T> ComputePredictorFeatures(
    const AdPredictorInfo<T>& ad_predictor,
    const ad_targeting::UserModelInfo& user_model,
    const AdEventList& ad_events) {
  AdPredictorInfo<T> mutable_ad_predictor = ad_predictor;

  const SegmentList intent_child_segments_intersection = SetIntersection(
      ad_targeting::GetTopParentChildPurchaseIntentSegments(user_model),
      ad_predictor.segments);
  mutable_ad_predictor.does_match_intent_child_segments =
      intent_child_segments_intersection.empty() ? false : true;

  const SegmentList intent_parent_segments_intersection = SetIntersection(
      ad_targeting::GetTopParentPurchaseIntenSegments(user_model),
      ad_predictor.segments);
  mutable_ad_predictor.does_match_intent_parent_segments =
      intent_parent_segments_intersection.empty() ? false : true;

  const SegmentList interest_child_segments_intersection = SetIntersection(
      ad_targeting::GetTopParentChildInterestSegments(user_model),
      ad_predictor.segments);
  mutable_ad_predictor.does_match_interest_child_segments =
      interest_child_segments_intersection.empty() ? false : true;

  const SegmentList interest_parent_segments_intersection =
      SetIntersection(ad_targeting::GetTopParentInterestSegments(user_model),
                      ad_predictor.segments);
  mutable_ad_predictor.does_match_interest_parent_segments =
      interest_parent_segments_intersection.empty() ? false : true;

  const base::Time now = base::Time::Now();

  const absl::optional<base::Time> last_seen_ad_at_optional =
      GetLastSeenAdTime(ad_events, ad_predictor.creative_ad);
  if (last_seen_ad_at_optional) {
    const base::Time last_seen_ad_at = last_seen_ad_at_optional.value();
    const base::TimeDelta time_delta = now - last_seen_ad_at;
    mutable_ad_predictor.ad_last_seen_hours_ago = time_delta.InHours();
  }

  const absl::optional<base::Time> last_seen_advertiser_at_optional =
      GetLastSeenAdvertiserTime(ad_events, ad_predictor.creative_ad);
  if (last_seen_advertiser_at_optional) {
    const base::Time last_seen_advertiser_at =
        last_seen_advertiser_at_optional.value();
    const base::TimeDelta time_delta = now - last_seen_advertiser_at;
    mutable_ad_predictor.advertiser_last_seen_hours_ago = time_delta.InHours();
  }

  return mutable_ad_predictor;
}

template <typename T>
double ComputePredictorScore(const AdPredictorInfo<T>& ad_predictor) {
  const AdPredictorWeights weights = features::GetAdPredictorWeights();
  double score = 0.0;

  if (ad_predictor.does_match_intent_child_segments) {
    score += weights.at(kDoesMatchIntentChildSegmentsIndex);
  } else if (ad_predictor.does_match_intent_parent_segments) {
    score += weights.at(kDoesMatchIntentParentSegmentsIndex);
  }

  if (ad_predictor.does_match_interest_child_segments) {
    score += weights.at(kDoesMatchInterestChildSegmentsIndex);
  } else if (ad_predictor.does_match_interest_parent_segments) {
    score += weights.at(kDoesMatchInterestParentSegmentsIndex);
  }

  if (ad_predictor.ad_last_seen_hours_ago <= base::Time::kHoursPerDay) {
    score += weights.at(AdLastSeenHoursAgoIndex) *
             ad_predictor.ad_last_seen_hours_ago /
             static_cast<double>(base::Time::kHoursPerDay);
  }

  if (ad_predictor.advertiser_last_seen_hours_ago <= base::Time::kHoursPerDay) {
    score += weights.at(kAdvertiserLastSeenHoursAgoIndex) *
             ad_predictor.advertiser_last_seen_hours_ago /
             static_cast<double>(base::Time::kHoursPerDay);
  }

  if (ad_predictor.creative_ad.priority > 0) {
    score += weights.at(kPriorityIndex) / ad_predictor.creative_ad.priority;
  }

  score *= ad_predictor.creative_ad.ptr;

  return score;
}

template <typename T>
CreativeAdPredictorMap<T> ComputePredictorFeaturesAndScores(
    const CreativeAdPredictorMap<T>& creative_ad_predictors,
    const ad_targeting::UserModelInfo& user_model,
    const AdEventList& ad_events) {
  CreativeAdPredictorMap<T> creative_ad_predictors_with_features;

  for (const auto& creative_ad_predictor : creative_ad_predictors) {
    AdPredictorInfo<T> ad_predictor = creative_ad_predictor.second;

    ad_predictor =
        ComputePredictorFeatures(ad_predictor, user_model, ad_events);
    ad_predictor.score = ComputePredictorScore(ad_predictor);

    creative_ad_predictors_with_features.insert(
        {ad_predictor.creative_ad.creative_instance_id, ad_predictor});
  }

  return creative_ad_predictors_with_features;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_PREDICTOR_UTIL_H_
