/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_HANDLER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_HANDLER_UTIL_H_

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

struct AdInfo;

bool HasFiredAdEvent(const AdInfo& ad,
                     const AdEventList& ad_events,
                     mojom::ConfirmationType mojom_confirmation_type);

// Set `time_window` to 0 to ignore the time window.
bool HasFiredAdEventWithinTimeWindow(
    const AdInfo& ad,
    const AdEventList& ad_events,
    mojom::ConfirmationType mojom_confirmation_type,
    base::TimeDelta time_window);

template <typename T>
bool WasAdServed(const AdInfo& ad,
                 const AdEventList& ad_events,
                 const T mojom_ad_event_type) {
  return mojom_ad_event_type == T::kServedImpression ||
         HasFiredAdEvent(ad, ad_events,
                         mojom::ConfirmationType::kServedImpression);
}

template <typename T>
bool ShouldDeduplicateViewedAdEvent(const AdInfo& ad,
                                    const AdEventList& ad_events,
                                    const T mojom_ad_event_type) {
  return mojom_ad_event_type == T::kViewedImpression &&
         HasFiredAdEventWithinTimeWindow(
             ad, ad_events, mojom::ConfirmationType::kViewedImpression,
             /*time_window=*/kDeduplicateViewedAdEventFor.Get());
}

template <typename T>
bool ShouldDeduplicateClickedAdEvent(const AdInfo& ad,
                                     const AdEventList& ad_events,
                                     const T mojom_ad_event_type) {
  return mojom_ad_event_type == T::kClicked &&
         HasFiredAdEventWithinTimeWindow(
             ad, ad_events, mojom::ConfirmationType::kClicked,
             /*time_window=*/kDeduplicateClickedAdEventFor.Get());
}

template <typename T>
bool ShouldDeduplicateAdEvent(const AdInfo& ad,
                              const AdEventList& ad_events,
                              const T mojom_ad_event_type) {
  return ShouldDeduplicateViewedAdEvent(ad, ad_events, mojom_ad_event_type) ||
         ShouldDeduplicateClickedAdEvent(ad, ad_events, mojom_ad_event_type);
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_HANDLER_UTIL_H_
