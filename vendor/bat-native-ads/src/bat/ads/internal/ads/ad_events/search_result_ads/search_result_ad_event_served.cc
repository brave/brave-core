/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/search_result_ads/search_result_ad_event_served.h"

#include "base/functional/bind.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"

namespace ads::search_result_ads {

void AdEventServed::FireEvent(const SearchResultAdInfo& ad) {
  BLOG(3, "Served search result ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kServed,
             base::BindOnce([](const bool success) {
               if (!success) {
                 BLOG(1, "Failed to log search result ad served event");
                 return;
               }

               BLOG(6, "Successfully logged search result ad served event");
             }));
}

}  // namespace ads::search_result_ads
