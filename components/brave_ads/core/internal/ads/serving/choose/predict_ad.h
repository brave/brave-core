/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/ads/serving/choose/ad_predictor_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/eligible_ads_predictor_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/choose/sample_ads.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pacing/pacing.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

template <typename T>
absl::optional<T> PredictAd(const UserModelInfo& user_model,
                            const AdEventList& ad_events,
                            const std::vector<T>& creative_ads) {
  CHECK(!creative_ads.empty());

  const std::vector<T> paced_creative_ads = PaceCreativeAds(creative_ads);

  CreativeAdPredictorMap<T> creative_ad_predictors;
  creative_ad_predictors =
      GroupCreativeAdsByCreativeInstanceId(paced_creative_ads);
  creative_ad_predictors = ComputePredictorFeaturesAndScores(
      creative_ad_predictors, user_model, ad_events);

  return SampleAdFromPredictors(creative_ad_predictors);
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_PREDICT_AD_H_
