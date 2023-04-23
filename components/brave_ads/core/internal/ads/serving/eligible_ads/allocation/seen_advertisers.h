/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADVERTISERS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADVERTISERS_H_

#include <map>
#include <string>

#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/allocation/round_robin_advertisers.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"

namespace brave_ads {

struct CreativeAdInfo;

template <typename T>
T FilterSeenAdvertisersAndRoundRobinIfNeeded(const T& ads, const AdType& type) {
  const std::map<std::string, bool>& seen_advertisers =
      ClientStateManager::GetInstance().GetSeenAdvertisersForType(type);

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

  ClientStateManager::GetInstance().ResetSeenAdvertisersForType(
      cast_creative_ads, type);

  return ads;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADVERTISERS_H_
