/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADS_H_

#include <map>
#include <string>

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/allocation/round_robin_ads.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads {

template <typename T>
T FilterSeenAdsAndRoundRobinIfNeeded(const T& ads, AdType ad_type) {
  const std::map<std::string, bool>& seen_ads =
      ClientStateManager::GetInstance().GetSeenAdsForType(ad_type);

  const T filtered_ads = FilterSeenAds(ads, seen_ads);
  if (!filtered_ads.empty()) {
    return filtered_ads;
  }

  BLOG(1, "All " << ad_type << " ads have been shown, so round robin");

  const CreativeAdList creative_ads{ads.cbegin(), ads.cend()};
  ClientStateManager::GetInstance().ResetSeenAdsForType(creative_ads, ad_type);

  return ads;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADS_H_
