/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_event_clicked.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/confirmations/confirmations.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace new_tab_page_ads {

AdEventClicked::AdEventClicked() = default;

AdEventClicked::~AdEventClicked() = default;

void AdEventClicked::FireEvent(const NewTabPageAdInfo& ad) {
  BLOG(3, "Clicked new tab page ad with uuid " << ad.uuid
                                               << " and creative instance id "
                                               << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kClicked, [](const bool success) {
    if (!success) {
      BLOG(1, "Failed to log new tab page ad clicked event");
      return;
    }

    BLOG(6, "Successfully logged new tab page ad clicked event");
  });

  history::AddNewTabPageAd(ad, ConfirmationType::kClicked);
}

}  // namespace new_tab_page_ads
}  // namespace ads
