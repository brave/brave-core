/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_factory.h"

#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_clicked.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_served.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_viewed.h"

namespace brave_ads {

std::unique_ptr<AdEventInterface<SearchResultAdInfo>>
SearchResultAdEventFactory::Build(
    mojom::SearchResultAdEventType mojom_ad_event_type) {
  switch (mojom_ad_event_type) {
    case mojom::SearchResultAdEventType::kServedImpression: {
      return std::make_unique<SearchResultAdEventServed>();
    }

    case mojom::SearchResultAdEventType::kViewedImpression: {
      return std::make_unique<SearchResultAdEventViewed>();
    }

    case mojom::SearchResultAdEventType::kClicked: {
      return std::make_unique<SearchResultAdEventClicked>();
    }
  }
}

}  // namespace brave_ads
