/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_H_

#include "bat/ads/internal/ad_pacing/ad_pacing_util.h"
#include "bat/ads/internal/eligible_ads/ad_notifications/filters/eligible_ads_filter_factory.h"

namespace ads {

struct CreativeAdInfo;

template <typename T>
T PaceAds(const T& ads) {
  T filtered_ads = ads;

  const auto iter =
      std::remove_if(filtered_ads.begin(), filtered_ads.end(),
                     [&](const CreativeAdInfo& ad) { return ShouldPace(ad); });

  filtered_ads.erase(iter, filtered_ads.end());

  // TODO(tmancey): Decouple priority filter, and implement like pacing
  //   const auto priority_filter =
  //       EligibleAdsFilterFactory::Build(EligibleAdsFilter::Type::kPriority);
  //   DCHECK(priority_filter);
  //   return priority_filter->Apply(paced_ads);

  return filtered_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_H_
