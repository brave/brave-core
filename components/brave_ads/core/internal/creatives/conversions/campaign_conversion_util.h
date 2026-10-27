/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_UTIL_H_

#include <cstddef>
#include <string>

#include "base/containers/span.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

class GURL;

namespace brave_ads {

using CampaignConversionBucketMap =
    absl::flat_hash_map</*campaign_id*/ std::string, CampaignConversionList>;

using CampaignConversionCountMap =
    absl::flat_hash_map</*campaign_id*/ std::string, /*count*/ size_t>;

CampaignConversionList GetMatchingCampaignConversions(
    const CampaignConversionList& campaign_conversions,
    base::span<const GURL> redirect_chain);

CampaignConversionCountMap GetCampaignConversionCounts(
    const AdEventList& ad_events);

CampaignConversionBucketMap SortCampaignConversionsIntoBuckets(
    const CampaignConversionList& campaign_conversions);

void FilterCampaignConversionBucketsThatExceedTheCap(
    const absl::flat_hash_map<std::string, size_t>& campaign_conversion_counts,
    size_t campaign_conversion_cap,
    CampaignConversionBucketMap& campaign_conversion_buckets);

bool HasCampaignConversionWithinObservationWindow(
    const CampaignConversionList& campaign_conversions,
    const AdEventInfo& ad_event);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_UTIL_H_
