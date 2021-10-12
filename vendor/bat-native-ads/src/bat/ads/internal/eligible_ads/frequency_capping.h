/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_FREQUENCY_CAPPING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_FREQUENCY_CAPPING_H_

#include <algorithm>
#include <iterator>

#include "base/check.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/internal/ads/exclusion_rules_base.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

template <typename T>
bool ShouldCapLastServedCreativeAd(const T& creative_ads) {
  return creative_ads.size() != 1;
}

template <typename T>
T ApplyFrequencyCapping(const T& creative_ads,
                        const AdInfo& last_served_ad,
                        ExclusionRulesBase* exclusion_rules) {
  DCHECK(exclusion_rules);

  const bool should_cap_last_served_ad =
      ShouldCapLastServedCreativeAd(creative_ads);

  T filtered_creative_ads;

  std::copy_if(creative_ads.cbegin(), creative_ads.cend(),
               std::back_inserter(filtered_creative_ads),
               [exclusion_rules, &last_served_ad,
                &should_cap_last_served_ad](const CreativeAdInfo& creative_ad) {
                 const bool should_exclude =
                     exclusion_rules->ShouldExcludeCreativeAd(creative_ad) ||
                     (should_cap_last_served_ad &&
                      creative_ad.creative_instance_id ==
                          last_served_ad.creative_instance_id);

                 return !should_exclude;
               });

  return filtered_creative_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_FREQUENCY_CAPPING_H_
