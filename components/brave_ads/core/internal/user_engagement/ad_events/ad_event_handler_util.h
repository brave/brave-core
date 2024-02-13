/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_HANDLER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_HANDLER_UTIL_H_

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads {

struct AdInfo;

bool HasFiredAdEvent(const AdInfo& ad,
                     const AdEventList& ad_events,
                     ConfirmationType confirmation_type);

template <typename T>
bool WasAdServed(const AdInfo& ad,
                 const AdEventList& ad_events,
                 const T& event_type) {
  return event_type == T::kServed ||
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kServed);
}

template <typename T>
bool ShouldDebounceViewedAdEvent(const AdInfo& ad,
                                 const AdEventList& ad_events,
                                 const T& event_type) {
  CHECK(WasAdServed(ad, ad_events, event_type));

  return event_type == T::kViewed &&
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed);
}

template <typename T>
bool ShouldDebounceClickedAdEvent(const AdInfo& ad,
                                  const AdEventList& ad_events,
                                  const T& event_type) {
  CHECK(WasAdServed(ad, ad_events, event_type));

  return event_type == T::kClicked &&
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kClicked);
}

template <typename T>
bool ShouldDebounceAdEvent(const AdInfo& ad,
                           const AdEventList& ad_events,
                           const T& event_type) {
  CHECK(WasAdServed(ad, ad_events, event_type));

  return ShouldDebounceViewedAdEvent(ad, ad_events, event_type) ||
         ShouldDebounceClickedAdEvent(ad, ad_events, event_type);
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_HANDLER_UTIL_H_
