/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_viewed.h"

#include "base/functional/bind.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads::new_tab_page_ads {

void AdEventViewed::FireEvent(const NewTabPageAdInfo& ad) {
  BLOG(3, "Viewed new tab page ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kViewed,
             base::BindOnce([](const bool success) {
               if (!success) {
                 BLOG(1, "Failed to log new tab page ad viewed event");
                 return;
               }

               BLOG(6, "Successfully logged new tab page ad viewed event");
             }));
}

}  // namespace ads::new_tab_page_ads
