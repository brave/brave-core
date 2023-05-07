/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_ELIGIBLE_ADS_PREDICTOR_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_ELIGIBLE_ADS_PREDICTOR_UTIL_H_

#include <cstddef>
#include <iterator>
#include <limits>
#include <vector>

#include "base/check_op.h"
#include "base/numerics/ranges.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/ad_predictor_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_alias.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_feature_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/top_segments.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

constexpr size_t kDoesMatchIntentChildSegmentsIndex = 0;
constexpr size_t kDoesMatchIntentParentSegmentsIndex = 1;
constexpr size_t kDoesMatchInterestChildSegmentsIndex = 2;
constexpr size_t kDoesMatchInterestParentSegmentsIndex = 3;
constexpr size_t kAdLastSeenHoursAgoIndex = 4;
constexpr size_t kAdvertiserLastSeenHoursAgoIndex = 5;
constexpr size_t kPriorityIndex = 6;

struct UserModelInfo;

SegmentList SegmentIntersection(SegmentList lhs, SegmentList rhs);

template <typename T>
CreativeAdPredictorMap<T> GroupCreativeAdsByCreativeInstanceId(
    const std::vector<T>& creative_ads) {
  CreativeAdPredictorMap<T> creative_ad_predictors;

  for (const auto& creative_ad : creative_ads) {
    const auto iter =
        creative_ad_predictors.find(creative_ad.creative_instance_id);
    if (iter != creative_ad_predictors.cend()) {
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
    const UserModelInfo& user_model,
    const AdEventList& ad_events) {
  AdPredictorInfo<T> mutable_ad_predictor(ad_predictor);

  const SegmentList intent_child_segments_intersection = SegmentIntersection(
      GetTopChildPurchaseIntentSegments(user_model), ad_predictor.segments);
  mutable_ad_predictor.does_match_intent_child_segments =
      !intent_child_segments_intersection.empty();

  const SegmentList intent_parent_segments_intersection = SegmentIntersection(
      GetTopParentPurchaseIntentSegments(user_model), ad_predictor.segments);
  mutable_ad_predictor.does_match_intent_parent_segments =
      !intent_parent_segments_intersection.empty();

  const SegmentList interest_child_segments_intersection = SegmentIntersection(
      GetTopChildInterestSegments(user_model), ad_predictor.segments);
  mutable_ad_predictor.does_match_interest_child_segments =
      !interest_child_segments_intersection.empty();

  const SegmentList interest_parent_segments_intersection = SegmentIntersection(
      GetTopParentInterestSegments(user_model), ad_predictor.segments);
  mutable_ad_predictor.does_match_interest_parent_segments =
      !interest_parent_segments_intersection.empty();

  const base::Time now = base::Time::Now();

  if (const absl::optional<base::Time> last_seen_ad_at =
          GetLastSeenAdTime(ad_events, ad_predictor.creative_ad)) {
    const base::TimeDelta time_delta = now - *last_seen_ad_at;
    mutable_ad_predictor.ad_last_seen_hours_ago = time_delta.InHours();
  }

  if (const absl::optional<base::Time> last_seen_advertiser_at =
          GetLastSeenAdvertiserTime(ad_events, ad_predictor.creative_ad)) {
    const base::TimeDelta time_delta = now - *last_seen_advertiser_at;
    mutable_ad_predictor.advertiser_last_seen_hours_ago = time_delta.InHours();
  }

  return mutable_ad_predictor;
}

template <typename T>
double ComputePredictorScore(const AdPredictorInfo<T>& ad_predictor) {
  AdPredictorWeightList ad_predictor_weights =
      ToAdPredictorWeights(kAdPredictorWeights.Get());
  if (ad_predictor_weights.empty()) {
    ad_predictor_weights =
        ToAdPredictorWeights(kAdPredictorWeights.default_value);
  }

  double score = 0.0;

  if (ad_predictor.does_match_intent_child_segments) {
    score += ad_predictor_weights.at(kDoesMatchIntentChildSegmentsIndex);
  } else if (ad_predictor.does_match_intent_parent_segments) {
    score += ad_predictor_weights.at(kDoesMatchIntentParentSegmentsIndex);
  }

  if (ad_predictor.does_match_interest_child_segments) {
    score += ad_predictor_weights.at(kDoesMatchInterestChildSegmentsIndex);
  } else if (ad_predictor.does_match_interest_parent_segments) {
    score += ad_predictor_weights.at(kDoesMatchInterestParentSegmentsIndex);
  }

  if (ad_predictor.ad_last_seen_hours_ago <= base::Time::kHoursPerDay) {
    score += ad_predictor_weights.at(kAdLastSeenHoursAgoIndex) *
             ad_predictor.ad_last_seen_hours_ago /
             static_cast<double>(base::Time::kHoursPerDay);
  }

  if (ad_predictor.advertiser_last_seen_hours_ago <= base::Time::kHoursPerDay) {
    score += ad_predictor_weights.at(kAdvertiserLastSeenHoursAgoIndex) *
             ad_predictor.advertiser_last_seen_hours_ago /
             static_cast<double>(base::Time::kHoursPerDay);
  }

  if (ad_predictor.creative_ad.priority > 0) {
    score += ad_predictor_weights.at(kPriorityIndex) /
             ad_predictor.creative_ad.priority;
  }

  return score;
}

template <typename T>
CreativeAdPredictorMap<T> ComputePredictorFeaturesAndScores(
    const CreativeAdPredictorMap<T>& creative_ad_predictors,
    const UserModelInfo& user_model,
    const AdEventList& ad_events) {
  CreativeAdPredictorMap<T> creative_ad_predictors_with_features;

  for (const auto& [segment, creative_ad_predictor] : creative_ad_predictors) {
    AdPredictorInfo<T> ad_predictor =
        ComputePredictorFeatures(creative_ad_predictor, user_model, ad_events);
    ad_predictor.score = ComputePredictorScore(ad_predictor);

    creative_ad_predictors_with_features.insert(
        {ad_predictor.creative_ad.creative_instance_id, ad_predictor});
  }

  return creative_ad_predictors_with_features;
}

template <typename T>
std::vector<int> ComputeVoteRegistry(
    const std::vector<T>& creative_ads,
    const TextEmbeddingHtmlEventList& text_embedding_html_events) {
  CHECK(!creative_ads.empty());

  std::vector<int> vote_registry(creative_ads.size());

  for (const auto& text_embedding_html_event : text_embedding_html_events) {
    std::vector<float> similarity_scores;

    for (const auto& creative_ad : creative_ads) {
      const ml::VectorData ad_embedding(creative_ad.embedding);
      const ml::VectorData page_text_embedding(
          text_embedding_html_event.embedding);
      const float similarity_score =
          ad_embedding.ComputeSimilarity(page_text_embedding);

      similarity_scores.push_back(similarity_score);
    }

    auto iter = base::ranges::max_element(
        similarity_scores.cbegin(), similarity_scores.cend(),
        [](const auto& lhs, const auto& rhs) { return lhs < rhs; });

    while (iter != similarity_scores.end()) {
      const size_t index = std::distance(similarity_scores.cbegin(), iter);
      CHECK_LT(index, vote_registry.size());
      vote_registry[index]++;

      iter = base::ranges::find_if(
          std::next(iter), similarity_scores.cend(), [iter](auto x) {
            return base::IsApproximatelyEqual(
                *iter, x, std::numeric_limits<float>::epsilon());
          });
    }
  }

  CHECK_EQ(vote_registry.size(), creative_ads.size());
  return vote_registry;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_ELIGIBLE_ADS_PREDICTOR_UTIL_H_
