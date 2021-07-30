/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PRIORITY_AD_PRIORITY_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PRIORITY_AD_PRIORITY_UTIL_H_

#include <map>
#include <utility>

#include "base/rand_util.h"

namespace ads {

template <typename T>
std::map<unsigned int, T> SortAdsIntoPrioritizedBuckets(const T& ads) {
  std::map<unsigned int, T> buckets;

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
    const std::map<unsigned int, T>& buckets) {
  const auto iter = std::min_element(
      buckets.begin(), buckets.end(),
      [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

  return *iter;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PRIORITY_AD_PRIORITY_UTIL_H_
