/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_H_

#include "base/containers/flat_map.h"
#include "bat/ads/internal/ads/serving/eligible_ads/priority/priority_util.h"
#include "bat/ads/internal/common/logging_util.h"

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

  int bucket_number = 1;
  for (const auto& [priority, ads] : buckets) {
    BLOG(3, ads.size() << " ads with a priority of " << priority
                       << " in bucket " << bucket_number);
    bucket_number++;
  }

  const auto& [priority, bucket] = GetHighestPriorityBucket(buckets);
  return bucket;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_H_
