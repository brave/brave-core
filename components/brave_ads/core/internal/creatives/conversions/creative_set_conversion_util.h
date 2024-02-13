/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_UTIL_H_

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"

class GURL;

namespace brave_ads {

using CreativeSetConversionBucketMap =
    std::map</*creative_set_id*/ std::string, CreativeSetConversionList>;

CreativeSetConversionList FilterConvertedAndNonMatchingCreativeSetConversions(
    const CreativeSetConversionList& creative_set_conversions,
    const AdEventList& ad_events,
    const std::vector<GURL>& redirect_chain);

CreativeSetConversionBucketMap SortCreativeSetConversionsIntoBuckets(
    const CreativeSetConversionList& creative_set_conversions);

std::optional<CreativeSetConversionInfo> FindNonExpiredCreativeSetConversion(
    const CreativeSetConversionList& creative_set_conversions,
    const AdEventInfo& ad_event);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_UTIL_H_
