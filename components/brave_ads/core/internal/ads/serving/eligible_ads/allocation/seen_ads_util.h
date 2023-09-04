/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADS_UTIL_H_

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

struct CreativeAdInfo;

absl::optional<base::Time> GetLastSeenAdAt(const AdEventList& ad_events,
                                           const CreativeAdInfo& creative_ad);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ALLOCATION_SEEN_ADS_UTIL_H_
