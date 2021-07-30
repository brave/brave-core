/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_UTIL_H_

#include "base/rand_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {

template <typename T>
bool ShouldPaceAd(const T& ad) {
  const double rand = base::RandDouble();
  if (rand <= ad.ptr) {
    return false;
  }

  BLOG(2, "Pacing delivery for creative instance id "
              << ad.creative_instance_id << " [Roll(" << ad.ptr << "):" << rand
              << "]");

  return true;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_UTIL_H_
