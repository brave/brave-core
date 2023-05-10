/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/search_result_ads/search_result_ad_event_factory.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/search_result_ads/search_result_ad_event_clicked.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/search_result_ads/search_result_ad_event_served.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/search_result_ads/search_result_ad_event_viewed.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_info.h"

namespace brave_ads {

std::unique_ptr<AdEventInterface<SearchResultAdInfo>>
SearchResultAdEventFactory::Build(
    const mojom::SearchResultAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  switch (event_type) {
    case mojom::SearchResultAdEventType::kServed: {
      return std::make_unique<SearchResultAdEventServed>();
    }

    case mojom::SearchResultAdEventType::kViewed: {
      return std::make_unique<SearchResultAdEventViewed>();
    }

    case mojom::SearchResultAdEventType::kClicked: {
      return std::make_unique<SearchResultAdEventClicked>();
    }
  }
}

}  // namespace brave_ads
