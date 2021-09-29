/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_UTIL_H_

#include "bat/ads/internal/ad_events/ad_event_info_aliases.h"

namespace absl {
template <typename T>
class optional;
}  // namespace absl

namespace base {
class Time;
}

namespace ads {

struct AdInfo;
struct CreativeAdInfo;

bool HasFiredAdViewedEvent(const AdInfo& ad, const AdEventList& ad_events);

absl::optional<base::Time> GetLastSeenAdTime(const AdEventList& ad_events,
                                             const CreativeAdInfo& creative_ad);

absl::optional<base::Time> GetLastSeenAdvertiserTime(
    const AdEventList& ad_events,
    const CreativeAdInfo& creative_ad);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_UTIL_H_
