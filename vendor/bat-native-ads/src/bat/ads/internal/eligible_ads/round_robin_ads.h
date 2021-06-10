/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ROUND_ROBIN_ADS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ROUND_ROBIN_ADS_H_

#include <map>
#include <string>

namespace ads {

struct CreativeAdInfo;

template <typename T>
T FilterSeenAds(const T& ads, const std::map<std::string, bool>& seen_ads) {
  T unseen_ads = ads;

  const auto iter = std::remove_if(
      unseen_ads.begin(), unseen_ads.end(), [&seen_ads](CreativeAdInfo& ad) {
        return seen_ads.find(ad.creative_instance_id) != seen_ads.end();
      });

  unseen_ads.erase(iter, unseen_ads.end());

  return unseen_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ROUND_ROBIN_ADS_H_
