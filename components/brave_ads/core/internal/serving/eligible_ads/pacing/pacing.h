/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PACING_PACING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PACING_PACING_H_

#include <vector>

#include "base/trace_event/trace_event.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing_util.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

struct CreativeAdInfo;

template <typename T>
void PaceCreativeAds(T& creative_ads) {
  TRACE_EVENT(kTraceEventCategory, "Pacing::PaceCreativeAds", "creative_ads",
              creative_ads.size());

  std::erase_if(creative_ads, [](const CreativeAdInfo& creative_ad) {
    return ShouldPaceAd(creative_ad);
  });
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PACING_PACING_H_
