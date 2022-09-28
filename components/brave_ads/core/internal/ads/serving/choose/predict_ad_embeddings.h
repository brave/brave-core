/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_EMBEDDINGS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_EMBEDDINGS_H_

#include <vector>

#include "absl/types/optional.h"
#include "bat/ads/internal/ads/serving/choose/eligible_ads_predictor_util.h"
#include "bat/ads/internal/ads/serving/choose/sample_ads.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pacing/pacing.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events.h"

#include <iostream>

namespace ads {

template <typename T> 
void PredictAdEmbeddings(
    const targeting::UserModelInfo& user_model,
    const AdEventList& ad_events,
    const std::vector<T>& creative_ads,
    std::function<void(const absl::optional<T>)> callback) {
    DCHECK(!creative_ads.empty());

    const std::vector<T> paced_creative_ads = PaceCreativeAds(creative_ads);

    GetTextEmbeddingHtmlEventsFromDatabase(
      [](const bool success,
         const TextEmbeddingHtmlEventList& text_embedding_html_events) {
        if (!success) return;

        const int text_embedding_html_event_count =
            text_embedding_html_events.size();
        std::cerr << "** Text Embedding Events Count: " << text_embedding_html_event_count;

        // Do Scoring and Matching here (placeholder below for now)
        CreativeAdPredictorMap<T> creative_ad_predictors;
        creative_ad_predictors =
            GroupCreativeAdsByCreativeInstanceId(paced_creative_ads);
        creative_ad_predictors = ComputePredictorFeaturesAndScores(
            creative_ad_predictors, user_model, ad_events);

        absl::optional<T> creative_ad = SampleAdFromPredictors(creative_ad_predictors);

        callback(creative_ad);
    });
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_EMBEDDINGS_H_
