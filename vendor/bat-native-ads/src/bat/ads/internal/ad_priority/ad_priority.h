/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PRIORITY_AD_PRIORITY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PRIORITY_AD_PRIORITY_H_

#include <map>
#include <utility>

#include "bat/ads/internal/ad_priority/ad_priority_util.h"

namespace ads {

template <typename T>
T PrioritizeAds(const T& ads) {
  if (ads.empty()) {
    return {};
  }

  const std::map<unsigned int, T> buckets = SortAdsIntoPrioritizedBuckets(ads);
  if (buckets.empty()) {
    return {};
  }

  const std::pair<unsigned int, T> bucket = GetHighestPriorityBucket(buckets);
  const unsigned int priority = bucket.first;
  const T creative_ads = bucket.second;

  BLOG(2, creative_ads.size()
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

  return creative_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PRIORITY_AD_PRIORITY_H_
