/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_SCORING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_SCORING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_SCORING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_SCORING_H_

#include <algorithm>
#include <iterator>
#include <vector>

#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_info.h"

namespace brave_ads {

template <typename T>
std::vector<double> ComputeCreativeAdSimilarityScores(
    const std::vector<T>& creative_ads,
    const TextEmbeddingHtmlEventInfo& text_embedding_html_event) {
  std::vector<double> creative_ad_similarity_scores;
  creative_ad_similarity_scores.reserve(creative_ads.size());

  const ml::VectorData text_embedding_html_event_embedding(
      text_embedding_html_event.embedding);

  std::transform(
      creative_ads.cbegin(), creative_ads.cend(),
      std::back_inserter(creative_ad_similarity_scores),
      [&text_embedding_html_event_embedding](const T& creative_ad) {
        const ml::VectorData creative_ad_embedding(creative_ad.embedding);
        return creative_ad_embedding.ComputeSimilarity(
            text_embedding_html_event_embedding);
      });

  return creative_ad_similarity_scores;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_SCORING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_SCORING_H_
