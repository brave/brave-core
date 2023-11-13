/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_CREATIVE_AD_MODEL_BASED_PREDICTOR_WEIGHTS_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_CREATIVE_AD_MODEL_BASED_PREDICTOR_WEIGHTS_BUILDER_H_

#include <type_traits>
#include <vector>

#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_ad_model_based_predictor_weights_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_inline_content_ad_model_based_predictor_weights_builder.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_new_tab_page_ad_model_based_predictor_weights_builder.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_notification_ad_model_based_predictor_weights_builder.h"

namespace brave_ads {

template <typename T>
CreativeAdModelBasedPredictorWeightsInfo
BuildCreativeAdModelBasedPredictorWeights(
    const std::vector<T>& /*creative_ad*/) {
  if constexpr (std::is_same_v<T, CreativeInlineContentAdInfo>) {
    return BuildCreativeInlineContentAdModelBasedPredictorWeights();
  }

  if constexpr (std::is_same_v<T, CreativeNewTabPageAdInfo>) {
    return BuildCreativeNewTabPageAdModelBasedPredictorWeights();
  }

  if constexpr (std::is_same_v<T, CreativeNotificationAdInfo>) {
    return BuildCreativeNotificationAdModelBasedPredictorWeights();
  }

  NOTREACHED_NORETURN() << "Unsupported creative ad";
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_WEIGHT_CREATIVE_AD_MODEL_BASED_PREDICTOR_WEIGHTS_BUILDER_H_
