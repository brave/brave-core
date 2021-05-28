/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/promoted_content_ads/promoted_content_ad_event_served.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/promoted_content_ad_info.h"

namespace ads {
namespace promoted_content_ads {

AdEventServed::AdEventServed() = default;

AdEventServed::~AdEventServed() = default;

void AdEventServed::FireEvent(const PromotedContentAdInfo& ad) {
  BLOG(3, "Served promoted content ad with uuid "
              << ad.uuid << " and creative instance id "
              << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kServed, [](const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to log promoted content ad served event");
      return;
    }

    BLOG(6, "Successfully logged promoted content ad served event");
  });
}

}  // namespace promoted_content_ads
}  // namespace ads
