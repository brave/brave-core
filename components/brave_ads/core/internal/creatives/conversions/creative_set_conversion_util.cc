/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_util.h"

#include <iterator>
#include <set>

#include "base/containers/contains.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/default_conversion/creative_set_conversion_url_pattern/creative_set_conversion_url_pattern_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/public/confirmation_type.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

std::set<std::string> GetConvertedCreativeSets(const AdEventList& ad_events) {
  std::set<std::string> creative_set_ids;

  for (const auto& ad_event : ad_events) {
    if (ad_event.confirmation_type == ConfirmationType::kConversion) {
      creative_set_ids.insert(ad_event.creative_set_id);
    }
  }

  return creative_set_ids;
}

}  // namespace

CreativeSetConversionList FilterConvertedAndNonMatchingCreativeSetConversions(
    const CreativeSetConversionList& creative_set_conversions,
    const AdEventList& ad_events,
    const std::vector<GURL>& redirect_chain) {
  if (creative_set_conversions.empty()) {
    return {};
  }

  const std::set<std::string> converted_creative_sets =
      GetConvertedCreativeSets(ad_events);

  CreativeSetConversionList filtered_creative_set_conversions;

  base::ranges::copy_if(
      creative_set_conversions,
      std::back_inserter(filtered_creative_set_conversions),
      [&converted_creative_sets, &redirect_chain](
          const CreativeSetConversionInfo& creative_set_conversion) {
        return !base::Contains(converted_creative_sets,
                               creative_set_conversion.id) &&
               DoesCreativeSetConversionUrlPatternMatchRedirectChain(
                   creative_set_conversion, redirect_chain);
      });

  return filtered_creative_set_conversions;
}

CreativeSetConversionBuckets SortCreativeSetConversionsIntoBuckets(
    const CreativeSetConversionList& creative_set_conversions) {
  CreativeSetConversionBuckets buckets;

  for (const auto& creative_set_conversion : creative_set_conversions) {
    buckets[creative_set_conversion.id].push_back(creative_set_conversion);
  }

  return buckets;
}

absl::optional<CreativeSetConversionInfo> FindNonExpiredCreativeSetConversion(
    const CreativeSetConversionList& creative_set_conversions,
    const AdEventInfo& ad_event) {
  const auto iter = base::ranges::find_if(
      creative_set_conversions,
      [&ad_event](const CreativeSetConversionInfo& creative_set_conversion) {
        return !HasObservationWindowForAdEventExpired(
            creative_set_conversion.observation_window, ad_event);
      });

  if (iter == creative_set_conversions.cend()) {
    return absl::nullopt;
  }

  return *iter;
}

}  // namespace brave_ads
