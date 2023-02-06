/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_UTIL_H_

#include <utility>

#include "base/containers/flat_map.h"
#include "base/ranges/algorithm.h"

namespace ads {

template <typename T>
base::flat_map<unsigned int, T> SortCreativeAdsIntoPrioritizedBuckets(
    const T& creative_ads) {
  base::flat_map<unsigned int, T> buckets;

  for (const auto& creative_ad : creative_ads) {
    if (creative_ad.priority == 0) {
      continue;
    }

    const auto iter = buckets.find(creative_ad.priority);
    if (iter == buckets.cend()) {
      buckets.insert({creative_ad.priority, {creative_ad}});
      continue;
    }

    iter->second.push_back(creative_ad);
  }

  return buckets;
}

template <typename T>
std::pair<unsigned int, T> GetHighestPriorityBucket(
    const base::flat_map<unsigned int, T>& buckets) {
  const auto iter = base::ranges::min_element(
      buckets,
      [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

  return *iter;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PRIORITY_PRIORITY_UTIL_H_
