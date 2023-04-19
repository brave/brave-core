/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/marked_to_no_longer_receive_exclusion_rule.h"

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

namespace brave_ads {

namespace {

bool DoesRespectCap(const CreativeAdInfo& creative_ad) {
  return !ShouldFilterSegment(creative_ad.segment);
}

}  // namespace

std::string MarkedToNoLongerReceiveExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.segment;
}

bool MarkedToNoLongerReceiveExclusionRule::ShouldExclude(
    const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(creative_ad)) {
    last_message_ = base::ReplaceStringPlaceholders(
        "creativeSetId $1 excluded due to $2 category being marked to no "
        "longer receive ads",
        {creative_ad.creative_set_id, creative_ad.segment}, nullptr);

    return true;
  }

  return false;
}

const std::string& MarkedToNoLongerReceiveExclusionRule::GetLastMessage()
    const {
  return last_message_;
}

}  // namespace brave_ads
