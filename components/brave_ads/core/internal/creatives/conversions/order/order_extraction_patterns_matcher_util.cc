/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_matcher_util.h"

#include <algorithm>

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util.h"
#include "brave/components/brave_ads/core/public/common/url/url_util.h"
#include "url/gurl.h"

namespace brave_ads {

bool HasPendingAdEvent(const CampaignConversionInfo& campaign_conversion,
                       const AdEventList& ad_events) {
  return std::ranges::any_of(
      ad_events, [&campaign_conversion](const AdEventInfo& ad_event) {
        return ad_event.campaign_id == campaign_conversion.id &&
               DidAdEventOccurWithinObservationWindow(
                   ad_event, campaign_conversion.observation_window);
      });
}

bool IsOrderExtractionEnabled(
    const CampaignConversionInfo& campaign_conversion) {
  return campaign_conversion.extract_id || campaign_conversion.extract_value;
}

bool MatchesRedirectChain(const CampaignConversionInfo& campaign_conversion,
                          base::span<const GURL> redirect_chain) {
  if (campaign_conversion.checkout_url_pattern &&
      MatchUrlPattern(redirect_chain,
                      *campaign_conversion.checkout_url_pattern)) {
    return true;
  }

  return MatchUrlPattern(redirect_chain,
                         campaign_conversion.thank_you_url_pattern);
}

}  // namespace brave_ads
