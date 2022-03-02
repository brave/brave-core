/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/search_result_ads/search_result_ad_event_clicked.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace search_result_ads {

AdEventClicked::AdEventClicked() = default;

AdEventClicked::~AdEventClicked() = default;

void AdEventClicked::FireEvent(const SearchResultAdInfo& ad) {
  BLOG(3, "Clicked search result ad with uuid " << ad.uuid
                                                << " and creative instance id "
                                                << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kClicked, [](const bool success) {
    if (!success) {
      BLOG(1, "Failed to log search result ad clicked event");
      return;
    }

    BLOG(6, "Successfully logged search result ad clicked event");
  });

  history::AddSearchResultAd(ad, ConfirmationType::kClicked);
}

}  // namespace search_result_ads
}  // namespace ads
