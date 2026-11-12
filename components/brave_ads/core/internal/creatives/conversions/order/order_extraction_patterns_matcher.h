/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_MATCHER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_MATCHER_H_

#include <optional>

#include "base/containers/span.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"

class GURL;

namespace brave_ads {

// Does not apply the conversion cap, since extraction is opportunistic and
// does not itself record a conversion.
std::optional<CampaignConversionInfo>
MaybeGetCampaignConversionForOrderExtraction(
    const CampaignConversionList& campaign_conversions,
    const AdEventList& ad_events,
    base::span<const GURL> redirect_chain);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_MATCHER_H_
