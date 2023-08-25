/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_factory.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_clicked.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_served.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_viewed.h"
#include "brave/components/brave_ads/core/public/ads/inline_content_ad_info.h"

namespace brave_ads {

std::unique_ptr<AdEventInterface<InlineContentAdInfo>>
InlineContentAdEventFactory::Build(
    const mojom::InlineContentAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  switch (event_type) {
    case mojom::InlineContentAdEventType::kServed: {
      return std::make_unique<InlineContentAdEventServed>();
    }

    case mojom::InlineContentAdEventType::kViewed: {
      return std::make_unique<InlineContentAdEventViewed>();
    }

    case mojom::InlineContentAdEventType::kClicked: {
      return std::make_unique<InlineContentAdEventClicked>();
    }
  }
}

}  // namespace brave_ads
