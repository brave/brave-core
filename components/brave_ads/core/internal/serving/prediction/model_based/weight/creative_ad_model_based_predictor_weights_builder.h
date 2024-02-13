/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_CREATIVE_AD_MODEL_BASED_PREDICTOR_WEIGHTS_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_CREATIVE_AD_MODEL_BASED_PREDICTOR_WEIGHTS_BUILDER_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_ad_model_based_predictor_weights_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_inline_content_ad_model_based_predictor_weights_builder.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_new_tab_page_ad_model_based_predictor_weights_builder.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_notification_ad_model_based_predictor_weights_builder.h"

namespace brave_ads {

template <typename T>
struct CreativeAdModelBasedPredictorWeightsBuilder;

template <>
struct CreativeAdModelBasedPredictorWeightsBuilder<
    CreativeInlineContentAdInfo> {
  static CreativeAdModelBasedPredictorWeightsInfo build() {
    return BuildCreativeInlineContentAdModelBasedPredictorWeights();
  }
};

template <>
struct CreativeAdModelBasedPredictorWeightsBuilder<CreativeNewTabPageAdInfo> {
  static CreativeAdModelBasedPredictorWeightsInfo build() {
    return BuildCreativeNewTabPageAdModelBasedPredictorWeights();
  }
};

template <>
struct CreativeAdModelBasedPredictorWeightsBuilder<CreativeNotificationAdInfo> {
  static CreativeAdModelBasedPredictorWeightsInfo build() {
    return BuildCreativeNotificationAdModelBasedPredictorWeights();
  }
};

template <typename T>
CreativeAdModelBasedPredictorWeightsInfo
BuildCreativeAdModelBasedPredictorWeights(
    const std::vector<T>& /*creative_ads*/) {
  return CreativeAdModelBasedPredictorWeightsBuilder<T>::build();
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_CREATIVE_AD_MODEL_BASED_PREDICTOR_WEIGHTS_BUILDER_H_
