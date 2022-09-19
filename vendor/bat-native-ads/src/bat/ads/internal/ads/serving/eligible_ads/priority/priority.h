/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_H_

#include <utility>

#include "base/containers/flat_map.h"
#include "bat/ads/internal/ads/serving/eligible_ads/priority/priority_util.h"
#include "bat/ads/internal/base/logging_util.h"

namespace ads {

template <typename T>
T PrioritizeCreativeAds(const T& creative_ads) {
  if (creative_ads.empty()) {
    return {};
  }

  const base::flat_map<unsigned int, T> buckets =
      SortCreativeAdsIntoPrioritizedBuckets(creative_ads);
  if (buckets.empty()) {
    return {};
  }

  const std::pair<unsigned int, T> highest_priority_bucket =
      GetHighestPriorityBucket(buckets);
  const unsigned int priority = highest_priority_bucket.first;
  const T prioritized_creative_ads = highest_priority_bucket.second;

  BLOG(2, prioritized_creative_ads.size()
              << " ads with a priority of " << priority << " in bucket 1");

  int index = 2;
  for (const auto& bucket : buckets) {
    if (bucket.first == priority) {
      continue;
    }

    BLOG(3, bucket.second.size() << " ads with a priority of " << bucket.first
                                 << " in bucket " << index);
    index++;
  }

  return prioritized_creative_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_H_
