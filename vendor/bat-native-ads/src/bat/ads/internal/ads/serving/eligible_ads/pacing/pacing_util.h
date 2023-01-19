/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PACING_PACING_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PACING_PACING_UTIL_H_

#include <ios>

#include "bat/ads/internal/ads/serving/eligible_ads/pacing/pacing_random_util.h"
#include "bat/ads/internal/common/logging_util.h"

namespace ads {

template <typename T>
bool ShouldPaceAd(const T& ad) {
  const double rand = GeneratePacingRandomNumber();
  if (rand < ad.ptr) {
    return false;
  }

  BLOG(2, std::fixed << "Pacing delivery for creative instance id "
                     << ad.creative_instance_id << " [Roll(" << ad.ptr
                     << "):" << rand << "]");

  return true;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PACING_PACING_UTIL_H_
