/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_EMBEDDINGS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_EMBEDDINGS_H_

#include <limits>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/numerics/ranges.h"
#include "base/rand_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/eligible_ads_predictor_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/sample_ads.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pacing/pacing.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

template <typename T>
absl::optional<T> MaybePredictAdUsingEmbeddings(
    const UserModelInfo& user_model,
    const std::vector<T>& creative_ads) {
  CHECK(!creative_ads.empty());

  const std::vector<T> paced_creative_ads = PaceCreativeAds(creative_ads);

  if (paced_creative_ads.empty()) {
    return absl::nullopt;
  }

  const std::vector<int> votes_registry = ComputeVoteRegistry(
      paced_creative_ads, user_model.text_embedding_html_events);

  const std::vector<double> probabilities =
      ComputeProbabilities(votes_registry);

  CHECK_EQ(paced_creative_ads.size(), probabilities.size());
  const double rand = base::RandDouble();
  double sum = 0.0;

  for (size_t i = 0; i < paced_creative_ads.size(); i++) {
    sum += probabilities.at(i);

    if (rand < sum && !base::IsApproximatelyEqual(
                          rand, sum, std::numeric_limits<double>::epsilon())) {
      return paced_creative_ads.at(i);
    }
  }

  return absl::nullopt;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_EMBEDDINGS_H_
