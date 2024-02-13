/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_AD_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_AD_UTIL_H_

#include <cstddef>

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_constants.h"

namespace brave_ads {

template <typename T>
size_t UntargetedCreativeAdCount(const T& creative_ads) {
  return base::ranges::count_if(
      creative_ads, [](const CreativeAdInfo& creative_ad) {
        return creative_ad.segment == kUntargetedSegment;
      });
}

template <typename T>
size_t TargetedCreativeAdCount(const T& creative_ads) {
  return base::ranges::count_if(
      creative_ads, [](const CreativeAdInfo& creative_ad) {
        return creative_ad.segment != kUntargetedSegment;
      });
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_AD_UTIL_H_
