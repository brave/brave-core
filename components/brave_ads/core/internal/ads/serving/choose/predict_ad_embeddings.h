/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_EMBEDDINGS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_EMBEDDINGS_H_

#include <iostream>
#include <vector>

#include "absl/types/optional.h"
#include "base/rand_util.h"
#include "bat/ads/internal/ads/serving/choose/eligible_ads_predictor_util.h"
#include "bat/ads/internal/ads/serving/choose/sample_ads.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pacing/pacing.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/numbers/number_util.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events.h"

namespace ads {

template <typename T>
void PredictAdEmbeddings(
    const std::vector<T>& creative_ads,
    std::function<void(const absl::optional<T>)> callback) {
  DCHECK(!creative_ads.empty());

  const std::vector<T> paced_creative_ads = PaceCreativeAds(creative_ads);

  GetTextEmbeddingHtmlEventsFromDatabase(
      [=](const bool success,
          const TextEmbeddingHtmlEventList& text_embedding_html_events) {
        if (!success) {
          BLOG(1, "Failed to get text embedding events");
          return;
        }

        const std::vector<int> votes_registry =
            ComputeVoteRegistry(paced_creative_ads, text_embedding_html_events);

        const std::vector<double> probabilities =
            ComputeProbabilities(votes_registry);

        const double rand = base::RandDouble();
        double sum = 0;

        for (size_t i = 0; i < paced_creative_ads.size(); i++) {
          sum += probabilities[i];

          if (DoubleIsLess(rand, sum)) {
            callback(paced_creative_ads.at(i));
          }
        }
      });
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_EMBEDDINGS_H_
