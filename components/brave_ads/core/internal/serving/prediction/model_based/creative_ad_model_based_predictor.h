/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_H_

#include <optional>
#include <vector>

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/sampling/creative_ad_model_based_predictor_sampling.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"

namespace brave_ads {

struct UserModelInfo;

template <typename T>
std::optional<T> MaybePredictCreativeAd(const std::vector<T>& creative_ads,
                                        const UserModelInfo& user_model,
                                        const AdEventList& ad_events) {
  CHECK(!creative_ads.empty());

  const CreativeAdModelBasedPredictorList<T> creative_ad_predictors =
      ComputeCreativeAdModelBasedPredictors(creative_ads, user_model,
                                            ad_events);

  return MaybeSampleCreativeAd(creative_ad_predictors);
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_H_
