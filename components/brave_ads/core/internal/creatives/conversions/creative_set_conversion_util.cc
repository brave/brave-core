/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_util.h"

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/default_conversion/creative_set_conversion_url_pattern/creative_set_conversion_url_pattern_util.h"
#include "url/gurl.h"

namespace brave_ads {

CreativeSetConversionList GetMatchingCreativeSetConversions(
    const CreativeSetConversionList& creative_set_conversions,
    const std::vector<GURL>& redirect_chain) {
  if (creative_set_conversions.empty()) {
    return {};
  }

  CreativeSetConversionList matching_creative_set_conversions;

  base::ranges::copy_if(
      creative_set_conversions,
      std::back_inserter(matching_creative_set_conversions),
      [&redirect_chain](
          const CreativeSetConversionInfo& creative_set_conversion) {
        return DoesCreativeSetConversionUrlPatternMatchRedirectChain(
            creative_set_conversion, redirect_chain);
      });

  return matching_creative_set_conversions;
}

CreativeSetConversionCountMap GetCreativeSetConversionCounts(
    const AdEventList& ad_events) {
  CreativeSetConversionCountMap creative_set_conversion_counts;

  for (const auto& ad_event : ad_events) {
    if (ad_event.confirmation_type == mojom::ConfirmationType::kConversion) {
      ++creative_set_conversion_counts[ad_event.creative_set_id];
    }
  }

  return creative_set_conversion_counts;
}

CreativeSetConversionBucketMap SortCreativeSetConversionsIntoBuckets(
    const CreativeSetConversionList& creative_set_conversions) {
  CreativeSetConversionBucketMap buckets;

  for (const auto& creative_set_conversion : creative_set_conversions) {
    buckets[creative_set_conversion.id].push_back(creative_set_conversion);
  }

  return buckets;
}

void FilterCreativeSetConversionBucketsThatExceedTheCap(
    const std::map<std::string, size_t>& creative_set_conversion_counts,
    size_t creative_set_conversion_cap,
    CreativeSetConversionBucketMap& creative_set_conversion_buckets) {
  if (creative_set_conversion_cap == 0) {
    // No cap.
    return;
  }

  for (const auto& [creative_set_id, creative_set_conversion_count] :
       creative_set_conversion_counts) {
    if (creative_set_conversion_count > creative_set_conversion_cap) {
      creative_set_conversion_buckets.erase(creative_set_id);
    }
  }
}

CreativeSetConversionList GetCreativeSetConversionsWithinObservationWindow(
    const CreativeSetConversionList& creative_set_conversions,
    const AdEventInfo& ad_event) {
  CreativeSetConversionList unexpired_creative_set_conversions;

  base::ranges::copy_if(
      creative_set_conversions,
      std::back_inserter(unexpired_creative_set_conversions),
      [&ad_event](const CreativeSetConversionInfo& creative_set_conversion) {
        return DidAdEventOccurWithinObservationWindow(
            ad_event, creative_set_conversion.observation_window);
      });

  return unexpired_creative_set_conversions;
}

}  // namespace brave_ads
