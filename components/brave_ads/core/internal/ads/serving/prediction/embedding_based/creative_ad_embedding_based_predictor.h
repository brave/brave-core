/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_EMBEDDING_BASED_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_EMBEDDING_BASED_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_H_

#include <vector>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ads/serving/prediction/embedding_based/creative_ad_embedding_based_predictor_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/prediction/embedding_based/sampling/creative_ad_embedding_based_predictor_sampling.h"
#include "brave/components/brave_ads/core/internal/ads/serving/prediction/embedding_based/voting/creative_ad_embedding_based_predictor_voting.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

template <typename T>
absl::optional<T> MaybePredictCreativeAd(const std::vector<T>& creative_ads,
                                         const UserModelInfo& user_model) {
  CHECK(!creative_ads.empty());

  const std::vector<int> creative_ad_vote_registry =
      ComputeCreativeAdVoteRegistryForTextEmbeddingHtmlEvents(
          creative_ads, user_model.text_embedding_html_events);

  const std::vector<double> creative_ad_probabilities =
      ComputeCreativeAdProbabilitiesForVoteRegistry(creative_ad_vote_registry);
  CHECK_EQ(creative_ads.size(), creative_ad_probabilities.size());

  return MaybeSampleCreativeAd(creative_ads, creative_ad_probabilities);
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_EMBEDDING_BASED_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_H_
