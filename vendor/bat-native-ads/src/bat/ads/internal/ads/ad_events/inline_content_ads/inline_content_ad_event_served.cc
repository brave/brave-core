/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_served.h"

#include "base/functional/bind.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/common/logging_util.h"

namespace ads::inline_content_ads {

void AdEventServed::FireEvent(const InlineContentAdInfo& ad) {
  BLOG(3, "Served inline content ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kServed,
             base::BindOnce([](const bool success) {
               if (!success) {
                 BLOG(1, "Failed to log inline content ad served event");
                 return;
               }

               BLOG(1, "Successfully logged inline content ad served event");
             }));
}

}  // namespace ads::inline_content_ads
