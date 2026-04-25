/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_util.h"

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_handler_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

namespace brave_ads {

bool IsAllowedToFireAdEvent() {
  return UserHasOptedInToNewTabPageAds();
}

bool ShouldFireAdEvent(const NewTabPageAdInfo& ad,
                       const AdEventList& ad_events,
                       mojom::NewTabPageAdEventType mojom_ad_event_type) {
  if (!WasAdServed(ad, ad_events, mojom_ad_event_type)) {
    BLOG(0,
         "New tab page ad: Not allowed because an ad was not served for "
         "placement id "
             << ad.placement_id);
    return false;
  }

  if (ShouldDeduplicateAdEvent(ad, ad_events, mojom_ad_event_type)) {
    BLOG(1, "New tab page ad: Not allowed as deduplicated "
                << mojom_ad_event_type << " event for placement id "
                << ad.placement_id);
    return false;
  }

  return true;
}

}  // namespace brave_ads
