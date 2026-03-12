/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ALLOCATION_ADS_ALLOCATION_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ALLOCATION_ADS_ALLOCATION_UTIL_H_

#include <cstdint>
#include <vector>

#include "base/check.h"
#include "base/rand_util.h"

namespace brave_ads {

// Chooses a uniformly random ad from `creative_ads`.
template <typename T>
T ChooseCreativeAdAtRandom(const std::vector<T>& creative_ads) {
  CHECK(!creative_ads.empty());

  const uint64_t rand = base::RandGenerator(creative_ads.size());
  return creative_ads.at(rand);
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ALLOCATION_ADS_ALLOCATION_UTIL_H_
