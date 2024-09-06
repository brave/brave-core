/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_handler_util.h"

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

namespace brave_ads {

bool HasFiredAdEvent(const AdInfo& ad,
                     const AdEventList& ad_events,
                     const mojom::ConfirmationType mojom_confirmation_type) {
  const auto iter = base::ranges::find_if(
      ad_events, [&ad, mojom_confirmation_type](const AdEventInfo& ad_event) {
        return ad_event.placement_id == ad.placement_id &&
               ad_event.confirmation_type == mojom_confirmation_type;
      });

  return iter != ad_events.cend();
}

bool HasFiredAdEventWithinTimeWindow(
    const AdInfo& ad,
    const AdEventList& ad_events,
    const mojom::ConfirmationType mojom_confirmation_type,
    const base::TimeDelta time_window) {
  const auto iter = base::ranges::find_if(
      ad_events,
      [&ad, mojom_confirmation_type, time_window](const AdEventInfo& ad_event) {
        CHECK(ad_event.created_at);

        return ad_event.placement_id == ad.placement_id &&
               ad_event.confirmation_type == mojom_confirmation_type &&
               (time_window.is_zero() ||
                base::Time::Now() - *ad_event.created_at <= time_window);
      });

  return iter != ad_events.cend();
}

}  // namespace brave_ads
