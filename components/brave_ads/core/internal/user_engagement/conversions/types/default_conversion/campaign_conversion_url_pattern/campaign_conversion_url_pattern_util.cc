/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/default_conversion/campaign_conversion_url_pattern/campaign_conversion_url_pattern_util.h"

#include "base/containers/span.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/public/common/url/url_util.h"
#include "url/gurl.h"

namespace brave_ads {

bool DoesCampaignConversionUrlPatternMatchRedirectChain(
    const CampaignConversionInfo& campaign_conversion,
    base::span<const GURL> redirect_chain) {
  return MatchUrlPattern(redirect_chain, campaign_conversion.url_pattern);
}

}  // namespace brave_ads
