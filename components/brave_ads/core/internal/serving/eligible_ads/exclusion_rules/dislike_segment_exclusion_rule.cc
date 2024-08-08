/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/dislike_segment_exclusion_rule.h"

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

namespace brave_ads {

namespace {

bool DoesRespectCap(const CreativeAdInfo& creative_ad) {
  return !ShouldFilterSegment(creative_ad.segment);
}

}  // namespace

std::string DislikeSegmentExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.segment;
}

base::expected<void, std::string> DislikeSegmentExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(creative_ad)) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "creativeSetId $1 excluded due to $2 segment being marked to no "
        "longer receive ads",
        {creative_ad.creative_set_id, creative_ad.segment}, nullptr));
  }

  return base::ok();
}

}  // namespace brave_ads
