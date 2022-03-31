/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ROUND_ROBIN_ADVERTISERS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ROUND_ROBIN_ADVERTISERS_H_

#include <algorithm>
#include <iterator>
#include <map>
#include <string>

#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

template <typename T>
T FilterSeenAdvertisers(const T& ads,
                        const std::map<std::string, bool>& seen_advertisers) {
  T unseen_advertisers;

  std::copy_if(ads.cbegin(), ads.cend(), std::back_inserter(unseen_advertisers),
               [&seen_advertisers](const CreativeAdInfo& creative_ad) {
                 return seen_advertisers.find(creative_ad.advertiser_id) ==
                        seen_advertisers.end();
               });

  return unseen_advertisers;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ROUND_ROBIN_ADVERTISERS_H_
