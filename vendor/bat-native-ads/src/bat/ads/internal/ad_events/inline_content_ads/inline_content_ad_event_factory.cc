/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/inline_content_ads/inline_content_ad_event_factory.h"

#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ad_events/inline_content_ads/inline_content_ad_event_clicked.h"
#include "bat/ads/internal/ad_events/inline_content_ads/inline_content_ad_event_served.h"
#include "bat/ads/internal/ad_events/inline_content_ads/inline_content_ad_event_viewed.h"

namespace ads {
namespace inline_content_ads {

std::unique_ptr<AdEvent<InlineContentAdInfo>> AdEventFactory::Build(
    const InlineContentAdEventType event_type) {
  switch (event_type) {
    case InlineContentAdEventType::kServed: {
      return std::make_unique<AdEventServed>();
    }

    case InlineContentAdEventType::kViewed: {
      return std::make_unique<AdEventViewed>();
    }

    case InlineContentAdEventType::kClicked: {
      return std::make_unique<AdEventClicked>();
    }
  }
}

}  // namespace inline_content_ads
}  // namespace ads
