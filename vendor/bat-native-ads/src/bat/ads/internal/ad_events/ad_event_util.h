/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_UTIL_H_

#include "bat/ads/internal/ad_events/ad_event_info_aliases.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

class ConfirmationType;
struct AdInfo;
struct CreativeAdInfo;

bool HasFiredAdEvent(const AdInfo& ad,
                     const AdEventList& ad_events,
                     const ConfirmationType& confirmation_type);

absl::optional<base::Time> GetLastSeenAdTime(const AdEventList& ad_events,
                                             const CreativeAdInfo& creative_ad);

absl::optional<base::Time> GetLastSeenAdvertiserTime(
    const AdEventList& ad_events,
    const CreativeAdInfo& creative_ad);

void PurgeExpiredAdEvents();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_UTIL_H_
