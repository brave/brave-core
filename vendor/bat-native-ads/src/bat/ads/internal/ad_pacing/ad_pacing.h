/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_H_

#include <algorithm>
#include <iterator>

#include "bat/ads/internal/ad_pacing/ad_pacing_util.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

template <typename T>
T PaceAds(const T& ads) {
  if (ads.empty()) {
    return {};
  }

  T paced_ads;

  std::copy_if(ads.cbegin(), ads.cend(), std::back_inserter(paced_ads),
               [&](const CreativeAdInfo& creative_ad) {
                 return !ShouldPaceAd(creative_ad);
               });

  return paced_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_H_
