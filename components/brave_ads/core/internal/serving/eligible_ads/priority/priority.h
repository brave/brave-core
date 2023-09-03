/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_H_

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/priority/priority_util.h"

namespace brave_ads {

template <typename T>
T PrioritizeCreativeAds(const T& creative_ads) {
  if (creative_ads.empty()) {
    return {};
  }

  const base::flat_map<int, T> buckets =
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

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_H_
