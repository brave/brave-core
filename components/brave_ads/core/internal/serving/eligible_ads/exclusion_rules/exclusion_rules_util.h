/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULES_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULES_UTIL_H_

#include "base/check.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rules_base.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

template <typename T>
bool CanCapLastServedCreativeAd(T& creative_ads) {
  return creative_ads.size() != 1;
}

template <typename T>
void ApplyExclusionRules(T& creative_ads,
                         const AdInfo& last_served_ad,
                         ExclusionRulesBase* exclusion_rules) {
  CHECK(exclusion_rules);

  TRACE_EVENT(kTraceEventCategory, "ApplyExclusionRules", "creative_ads",
              creative_ads.size());

  if (creative_ads.empty()) {
    return;
  }

  const bool can_cap_last_served_creative_ad =
      CanCapLastServedCreativeAd(creative_ads);

  std::erase_if(creative_ads, [exclusion_rules, &last_served_ad,
                               &can_cap_last_served_creative_ad](
                                  const CreativeAdInfo& creative_ad) {
    return exclusion_rules->ShouldExcludeCreativeAd(creative_ad) ||
           (can_cap_last_served_creative_ad &&
            creative_ad.creative_instance_id ==
                last_served_ad.creative_instance_id);
  });
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULES_UTIL_H_
