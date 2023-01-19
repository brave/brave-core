/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/marked_to_no_longer_receive_exclusion_rule.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/segments/segment_util.h"

namespace ads {

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
    last_message_ = base::StringPrintf(
        "creativeSetId %s excluded due to %s category being marked to no "
        "longer receive ads",
        creative_ad.creative_set_id.c_str(), creative_ad.segment.c_str());

    return true;
  }

  return false;
}

const std::string& MarkedToNoLongerReceiveExclusionRule::GetLastMessage()
    const {
  return last_message_;
}

}  // namespace ads
