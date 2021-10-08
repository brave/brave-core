/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_UTIL_H_

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "bat/ads/internal/bundle/creative_ad_info_aliases.h"
#include "bat/ads/internal/eligible_ads/ad_predictor_info.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_util_aliases.h"

namespace ads {

template <typename T>
bool ShouldCapLastServedAd(const T& ads) {
  return ads.size() != 1;
}

template <typename T>
std::map<std::string, AdPredictorInfo<T>> GroupCreativeAdsByCreativeInstanceId(
    const std::vector<T>& creative_ads) {
  std::map<std::string, AdPredictorInfo<T>> grouped_creative_ads;

  for (const auto& creative_ad : creative_ads) {
    const auto iter =
        grouped_creative_ads.find(creative_ad.creative_instance_id);
    if (iter != grouped_creative_ads.end()) {
      iter->second.segments.push_back(creative_ad.segment);
      continue;
    }

    AdPredictorInfo<T> ad_predictor;
    ad_predictor.segments = {creative_ad.segment};
    ad_predictor.creative_ad = creative_ad;

    grouped_creative_ads.insert(
        {creative_ad.creative_instance_id, ad_predictor});
  }

  return grouped_creative_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_UTIL_H_
