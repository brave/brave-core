/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_util.h"

#include <algorithm>

#include "base/containers/span.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/default_conversion/campaign_conversion_url_pattern/campaign_conversion_url_pattern_util.h"
#include "url/gurl.h"

namespace brave_ads {

CampaignConversionList GetMatchingCampaignConversions(
    const CampaignConversionList& campaign_conversions,
    base::span<const GURL> redirect_chain) {
  if (campaign_conversions.empty()) {
    return {};
  }

  CampaignConversionList matching_campaign_conversions;

  std::ranges::copy_if(
      campaign_conversions, std::back_inserter(matching_campaign_conversions),
      [&redirect_chain](const CampaignConversionInfo& campaign_conversion) {
        return DoesCampaignConversionUrlPatternMatchRedirectChain(
            campaign_conversion, redirect_chain);
      });

  return matching_campaign_conversions;
}

CampaignConversionCountMap GetCampaignConversionCounts(
    const AdEventList& ad_events) {
  CampaignConversionCountMap campaign_conversion_counts;

  for (const auto& ad_event : ad_events) {
    if (ad_event.confirmation_type == mojom::ConfirmationType::kConversion) {
      ++campaign_conversion_counts[ad_event.campaign_id];
    }
  }

  return campaign_conversion_counts;
}

CampaignConversionBucketMap SortCampaignConversionsIntoBuckets(
    const CampaignConversionList& campaign_conversions) {
  CampaignConversionBucketMap buckets;

  for (const auto& campaign_conversion : campaign_conversions) {
    buckets[campaign_conversion.id].push_back(campaign_conversion);
  }

  return buckets;
}

void FilterCampaignConversionBucketsThatExceedTheCap(
    const absl::flat_hash_map<std::string, size_t>& campaign_conversion_counts,
    size_t campaign_conversion_cap,
    CampaignConversionBucketMap& campaign_conversion_buckets) {
  if (campaign_conversion_cap == 0) {
    // No cap.
    return;
  }

  for (const auto& [campaign_id, campaign_conversion_count] :
       campaign_conversion_counts) {
    if (campaign_conversion_count > campaign_conversion_cap) {
      campaign_conversion_buckets.erase(campaign_id);
    }
  }
}

bool HasCampaignConversionWithinObservationWindow(
    const CampaignConversionList& campaign_conversions,
    const AdEventInfo& ad_event) {
  return std::ranges::any_of(
      campaign_conversions,
      [&ad_event](const CampaignConversionInfo& campaign_conversion) {
        return DidAdEventOccurWithinObservationWindow(
            ad_event, campaign_conversion.observation_window);
      });
}

}  // namespace brave_ads
