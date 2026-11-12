/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_matcher.h"

#include <algorithm>

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_matcher_util.h"

namespace brave_ads {

std::optional<CampaignConversionInfo>
MaybeGetCampaignConversionForOrderExtraction(
    const CampaignConversionList& campaign_conversions,
    const AdEventList& ad_events,
    base::span<const GURL> redirect_chain) {
  const auto iter = std::ranges::find_if(
      campaign_conversions,
      [&ad_events,
       &redirect_chain](const CampaignConversionInfo& campaign_conversion) {
        return IsOrderExtractionEnabled(campaign_conversion) &&
               MatchesRedirectChain(campaign_conversion, redirect_chain) &&
               HasPendingAdEvent(campaign_conversion, ad_events);
      });
  if (iter == campaign_conversions.cend()) {
    return std::nullopt;
  }

  return *iter;
}

}  // namespace brave_ads
