/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_VOTING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_VOTING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_VOTING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_VOTING_UTIL_H_

#include <vector>

namespace brave_ads {

void ComputeCreativeAdVoteRegistryForSimilarityScores(
    const std::vector<double>& creative_ad_similarity_scores,
    std::vector<int>& creative_ad_vote_registry);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_EMBEDDING_BASED_VOTING_CREATIVE_AD_EMBEDDING_BASED_PREDICTOR_VOTING_UTIL_H_
