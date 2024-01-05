/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_UTIL_H_

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_constants.h"

namespace brave_ads {

template <typename T>
size_t UntargetedCreativeAdCount(const T& creative_ads) {
  return base::ranges::count_if(
      creative_ads, [](const CreativeAdInfo& creative_ad) {
        return creative_ad.segment == kUntargetedSegment;
      });
}

template <typename T>
void LogNumberOfUntargetedCreativeAdsForBucket(const T& creative_ads,
                                               const int priority,
                                               const int bucket) {
  const size_t count = UntargetedCreativeAdCount(creative_ads);
  if (count > 0) {
    BLOG(3, count << " untargeted ads with a priority of " << priority
                  << " in bucket " << bucket);
  }
}

template <typename T>
void LogNumberOfTargetedCreativeAdsForBucket(const T& creative_ads,
                                             const int priority,
                                             const int bucket) {
  const size_t count =
      creative_ads.size() - UntargetedCreativeAdCount(creative_ads);
  if (count > 0) {
    BLOG(3, count << " targeted ads with a priority of " << priority
                  << " in bucket " << bucket);
  }
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_UTIL_H_
