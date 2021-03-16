/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_UTIL_H_

#include <cstdint>
#include <map>
#include <string>

#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

template <typename T>
T FilterSeenAdvertisers(
    const T& ads,
    const std::map<std::string, uint64_t>& seen_advertisers) {
  T unseen_advertisers = ads;

  const auto iter =
      std::remove_if(unseen_advertisers.begin(), unseen_advertisers.end(),
                     [&seen_advertisers](CreativeAdInfo& ad) {
                       return seen_advertisers.find(ad.advertiser_id) !=
                              seen_advertisers.end();
                     });

  unseen_advertisers.erase(iter, unseen_advertisers.end());

  return unseen_advertisers;
}

template <typename T>
T FilterSeenAds(const T& ads, const std::map<std::string, uint64_t>& seen_ads) {
  T unseen_ads = ads;

  const auto iter = std::remove_if(
      unseen_ads.begin(), unseen_ads.end(), [&seen_ads](CreativeAdInfo& ad) {
        return seen_ads.find(ad.creative_instance_id) != seen_ads.end();
      });

  unseen_ads.erase(iter, unseen_ads.end());

  return unseen_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_UTIL_H_
