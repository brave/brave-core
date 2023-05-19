/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_served.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"

namespace brave_ads {

void NewTabPageAdEventServed::FireEvent(const NewTabPageAdInfo& ad) {
  BLOG(3, "Served new tab page ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kServed,
             base::BindOnce([](const bool success) {
               if (!success) {
                 return BLOG(1, "Failed to log new tab page ad served event");
               }

               BLOG(6, "Successfully logged new tab page ad served event");
             }));
}

}  // namespace brave_ads
