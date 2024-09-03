/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_UTIL_H_

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

struct SearchResultAdInfo;

void MaybeBuildAndSaveCreativeSetConversion(
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad,
    mojom::SearchResultAdEventType mojom_ad_event_type);

bool IsAllowedToFireAdEvent(
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad,
    mojom::SearchResultAdEventType mojom_ad_event_type);

bool ShouldFireAdEvent(const SearchResultAdInfo& ad,
                       const AdEventList& ad_events,
                       mojom::SearchResultAdEventType mojom_ad_event_type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_UTIL_H_
