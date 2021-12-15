/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_

#include <string>

#include "bat/ads/internal/ad_events/ad_event_info_aliases.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

class AdType;
class ConfirmationType;
struct AdEventInfo;
struct CreativeAdInfo;

AdEventInfo BuildAdEvent(const CreativeAdInfo& creative_ad,
                         const AdType& ad_type,
                         const ConfirmationType& confirmation_type,
                         const base::Time& created_at);
AdEventInfo BuildAdEvent(const std::string& uuid,
                         const std::string& creative_set_id,
                         const ConfirmationType& confirmation_type);
AdEventInfo BuildAdEvent(const std::string& creative_set_id,
                         const ConfirmationType& confirmation_type);

void FireAdEvent(const AdEventInfo& ad_event);
void FireAdEvents(const AdEventInfo& ad_event, const int count);

int GetAdEventCount(const AdType& ad_type,
                    const ConfirmationType& confirmation_type,
                    const AdEventList& ad_events);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_UNITTEST_UTIL_H_
