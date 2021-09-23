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

#include "bat/ads/internal/eligible_ads/ad_predictor_info.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_util_aliases.h"

namespace ads {

struct CreativeAdNotificationInfo;
struct CreativeInlineContentAdInfo;

template <typename T>
std::map<std::string, AdPredictorInfo<T>> GroupEligibleAdsByCreativeInstanceId(
    const std::vector<T>& eligible_ads) {
  std::map<std::string, AdPredictorInfo<T>> ads;
  for (const auto& eligible_ad : eligible_ads) {
    const auto iter = ads.find(eligible_ad.creative_instance_id);
    if (iter != ads.end()) {
      iter->second.segments.push_back(eligible_ad.segment);
      continue;
    }

    AdPredictorInfo<T> ad_predictor;
    ad_predictor.segments = {eligible_ad.segment};
    ad_predictor.creative_ad = eligible_ad;
    ads.insert({eligible_ad.creative_instance_id, ad_predictor});
  }

  return ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_UTIL_H_
