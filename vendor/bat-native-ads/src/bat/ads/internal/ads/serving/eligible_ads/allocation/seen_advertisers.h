/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADVERTISERS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADVERTISERS_H_

#include <map>
#include <string>

#include "bat/ads/internal/ads/serving/eligible_ads/allocation/round_robin_advertisers.h"
#include "bat/ads/internal/base/logging_util.h"

namespace ads {

class ClientStateManager;
struct CreativeAdInfo;

template <typename T>
T FilterSeenAdvertisersAndRoundRobinIfNeeded(const T& ads, const AdType& type) {
  const std::map<std::string, bool> seen_advertisers =
      ClientStateManager::GetInstance()->GetSeenAdvertisersForType(type);

  const T filtered_ads = FilterSeenAdvertisers(ads, seen_advertisers);
  if (!filtered_ads.empty()) {
    return filtered_ads;
  }

  BLOG(1, "All " << type << " advertisers have been shown, so round robin");

  CreativeAdList cast_creative_ads;
  for (const auto& ad : ads) {
    const CreativeAdInfo cast_creative_ad = static_cast<CreativeAdInfo>(ad);
    cast_creative_ads.push_back(cast_creative_ad);
  }

  ClientStateManager::GetInstance()->ResetSeenAdvertisersForType(
      cast_creative_ads, type);

  return ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADVERTISERS_H_
