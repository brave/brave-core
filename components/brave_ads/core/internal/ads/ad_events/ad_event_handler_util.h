/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_AD_EVENT_HANDLER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_AD_EVENT_HANDLER_UTIL_H_

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"

namespace brave_ads {

class ConfirmationType;
struct AdInfo;

bool HasFiredAdEvent(const AdInfo& ad,
                     const AdEventList& ad_events,
                     const ConfirmationType& confirmation_type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_AD_EVENT_HANDLER_UTIL_H_
