/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/inline_content_ads/inline_content_ad_event_viewed.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/p2a/p2a_ad_impressions/p2a_ad_impression.h"

namespace ads {
namespace inline_content_ads {

AdEventViewed::AdEventViewed() = default;

AdEventViewed::~AdEventViewed() = default;

void AdEventViewed::FireEvent(const InlineContentAdInfo& ad) {
  BLOG(3, "Viewed inline content ad with uuid " << ad.uuid
                                                << " and creative instance id "
                                                << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kViewed, [](const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to log inline content ad viewed event");
      return;
    }

    BLOG(6, "Successfully logged inline content ad viewed event");
  });

  history::AddInlineContentAd(ad, ConfirmationType::kViewed);

  p2a::RecordAdImpression(ad);
}

}  // namespace inline_content_ads
}  // namespace ads
