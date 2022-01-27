/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_CHOOSE_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_CHOOSE_AD_H_

#include <vector>

#include "bat/ads/internal/ad_pacing/ad_pacing.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_aliases.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_predictor_util.h"
#include "bat/ads/internal/eligible_ads/sample_ads.h"

namespace absl {
template <typename T>
class optional;
}  // namespace absl

namespace ads {

template <typename T>
absl::optional<T> ChooseAd(const ad_targeting::UserModelInfo& user_model,
                           const AdEventList& ad_events,
                           const std::vector<T>& creative_ads) {
  DCHECK(!creative_ads.empty());

  const std::vector<T>& paced_creative_ads = PaceAds(creative_ads);

  CreativeAdPredictorMap<T> creative_ad_predictors;
  creative_ad_predictors =
      GroupCreativeAdsByCreativeInstanceId(paced_creative_ads);
  creative_ad_predictors = ComputePredictorFeaturesAndScores(
      creative_ad_predictors, user_model, ad_events);

  return SampleAdFromPredictors(creative_ad_predictors);
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_CHOOSE_AD_H_
