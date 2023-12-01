/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ALLOCATION_ROUND_ROBIN_ADS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ALLOCATION_ROUND_ROBIN_ADS_H_

#include <iterator>
#include <map>
#include <string>

#include "base/containers/contains.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

namespace brave_ads {

template <typename T>
T FilterSeenAds(const T& ads, const std::map<std::string, bool>& seen_ads) {
  T unseen_ads;

  base::ranges::copy_if(ads, std::back_inserter(unseen_ads),
                        [&seen_ads](const CreativeAdInfo& creative_ad) {
                          return !base::Contains(
                              seen_ads, creative_ad.creative_instance_id);
                        });

  return unseen_ads;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ALLOCATION_ROUND_ROBIN_ADS_H_
