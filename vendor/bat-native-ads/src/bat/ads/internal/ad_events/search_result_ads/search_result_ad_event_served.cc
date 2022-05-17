/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/search_result_ads/search_result_ad_event_served.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/history/history.h"

namespace ads {
namespace search_result_ads {

AdEventServed::AdEventServed() = default;

AdEventServed::~AdEventServed() = default;

void AdEventServed::FireEvent(const SearchResultAdInfo& ad) {
  BLOG(3, "Served search result ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kServed, [](const bool success) {
    if (!success) {
      BLOG(1, "Failed to log search result ad served event");
      return;
    }

    BLOG(6, "Successfully logged search result ad served event");
  });
}

}  // namespace search_result_ads
}  // namespace ads
