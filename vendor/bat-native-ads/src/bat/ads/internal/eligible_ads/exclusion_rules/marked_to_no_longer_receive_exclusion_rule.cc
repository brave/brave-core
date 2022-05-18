/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/exclusion_rules/marked_to_no_longer_receive_exclusion_rule.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/segments/segments_util.h"

namespace ads {

MarkedToNoLongerReceiveExclusionRule::MarkedToNoLongerReceiveExclusionRule() =
    default;

MarkedToNoLongerReceiveExclusionRule::~MarkedToNoLongerReceiveExclusionRule() =
    default;

std::string MarkedToNoLongerReceiveExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.segment;
}

bool MarkedToNoLongerReceiveExclusionRule::ShouldExclude(
    const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s excluded due to %s category being marked to no "
        "longer receive ads",
        creative_ad.creative_set_id.c_str(), creative_ad.segment.c_str());

    return true;
  }

  return false;
}

std::string MarkedToNoLongerReceiveExclusionRule::GetLastMessage() const {
  return last_message_;
}

bool MarkedToNoLongerReceiveExclusionRule::DoesRespectCap(
    const CreativeAdInfo& creative_ad) {
  if (ShouldFilterSegment(creative_ad.segment)) {
    return false;
  }

  return true;
}

}  // namespace ads
