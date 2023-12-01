/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_VOTING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_VOTING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_VOTING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_VOTING_H_

#include <vector>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/scoring/creative_ad_embedding_based_predictor_scoring.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/voting/creative_ad_embedding_based_predictor_voting_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_info.h"

namespace brave_ads {

template <typename T>
std::vector<int> ComputeCreativeAdVoteRegistryForTextEmbeddingHtmlEvents(
    const std::vector<T>& creative_ads,
    const TextEmbeddingHtmlEventList& text_embedding_html_events) {
  CHECK(!creative_ads.empty());

  std::vector<int> creative_ad_vote_registry(creative_ads.size());

  for (const auto& text_embedding_html_event : text_embedding_html_events) {
    const std::vector<double> creative_ad_similarity_scores =
        ComputeCreativeAdSimilarityScores(creative_ads,
                                          text_embedding_html_event);

    ComputeCreativeAdVoteRegistryForSimilarityScores(
        creative_ad_similarity_scores, creative_ad_vote_registry);
  }

  return creative_ad_vote_registry;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_VOTING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_VOTING_H_
