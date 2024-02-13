/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/voting/creative_ad_embedding_based_predictor_voting_util.h"

#include <cstddef>
#include <iterator>
#include <limits>

#include "base/check_op.h"
#include "base/numerics/ranges.h"
#include "base/ranges/algorithm.h"

namespace brave_ads {

void ComputeCreativeAdVoteRegistryForSimilarityScores(
    const std::vector<double>& creative_ad_similarity_scores,
    std::vector<int>& creative_ad_vote_registry) {
  CHECK_EQ(creative_ad_similarity_scores.size(),
           creative_ad_vote_registry.size());

  auto iter = base::ranges::max_element(
      creative_ad_similarity_scores,
      [](const auto& lhs, const auto& rhs) { return lhs < rhs; });

  while (iter != creative_ad_similarity_scores.cend()) {
    const size_t index =
        std::distance(creative_ad_similarity_scores.cbegin(), iter);
    CHECK_LT(index, creative_ad_vote_registry.size());

    ++creative_ad_vote_registry[index];

    iter = base::ranges::find_if(
        std::next(iter), creative_ad_similarity_scores.cend(),
        [iter](const double similarity_score) {
          return base::IsApproximatelyEqual(
              *iter, similarity_score, std::numeric_limits<double>::epsilon());
        });
  }
}

}  // namespace brave_ads
