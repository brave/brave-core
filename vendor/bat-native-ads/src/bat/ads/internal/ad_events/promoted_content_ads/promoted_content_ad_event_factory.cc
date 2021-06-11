/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/promoted_content_ads/promoted_content_ad_event_factory.h"

#include "bat/ads/internal/ad_events/promoted_content_ads/promoted_content_ad_event_clicked.h"
#include "bat/ads/internal/ad_events/promoted_content_ads/promoted_content_ad_event_served.h"
#include "bat/ads/internal/ad_events/promoted_content_ads/promoted_content_ad_event_viewed.h"
#include "bat/ads/promoted_content_ad_info.h"

namespace ads {
namespace promoted_content_ads {

std::unique_ptr<AdEvent<PromotedContentAdInfo>> AdEventFactory::Build(
    const PromotedContentAdEventType event_type) {
  switch (event_type) {
    case PromotedContentAdEventType::kServed: {
      return std::make_unique<AdEventServed>();
    }

    case PromotedContentAdEventType::kViewed: {
      return std::make_unique<AdEventViewed>();
    }

    case PromotedContentAdEventType::kClicked: {
      return std::make_unique<AdEventClicked>();
    }
  }
}

}  // namespace promoted_content_ads
}  // namespace ads
