/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_MATCHER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_MATCHER_UTIL_H_

#include "base/containers/span.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"

class GURL;

namespace brave_ads {

// Returns true if there is a pending ad event for the given campaign
// conversion.
bool HasPendingAdEvent(const CampaignConversionInfo& campaign_conversion,
                       const AdEventList& ad_events);

// Returns true if order extraction is enabled for the given campaign
// conversion.
bool IsOrderExtractionEnabled(
    const CampaignConversionInfo& campaign_conversion);

// Returns true if the redirect chain matches the checkout or thank-you URL
// pattern.
bool MatchesRedirectChain(const CampaignConversionInfo& campaign_conversion,
                          base::span<const GURL> redirect_chain);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_MATCHER_UTIL_H_
