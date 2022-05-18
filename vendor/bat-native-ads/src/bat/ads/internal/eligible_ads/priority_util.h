/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_PRIORITY_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_PRIORITY_UTIL_H_

#include <algorithm>
#include <utility>

#include "base/containers/flat_map.h"

namespace ads {

template <typename T>
base::flat_map<unsigned int, T> SortAdsIntoPrioritizedBuckets(const T& ads) {
  base::flat_map<unsigned int, T> buckets;

  for (const auto& ad : ads) {
    if (ad.priority == 0) {
      continue;
    }

    const auto iter = buckets.find(ad.priority);
    if (iter == buckets.end()) {
      buckets.insert({ad.priority, {ad}});
      continue;
    }

    iter->second.push_back(ad);
  }

  return buckets;
}

template <typename T>
std::pair<unsigned int, T> GetHighestPriorityBucket(
    const base::flat_map<unsigned int, T>& buckets) {
  const auto iter = std::min_element(
      buckets.cbegin(), buckets.cend(),
      [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

  return *iter;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_PRIORITY_UTIL_H_
