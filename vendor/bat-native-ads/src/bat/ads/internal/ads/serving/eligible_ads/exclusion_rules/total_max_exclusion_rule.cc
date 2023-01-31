/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/total_max_exclusion_rule.h"

#include <utility>

#include "base/ranges/algorithm.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"

namespace ads {

namespace {

bool DoesRespectCap(const AdEventList& ad_events,
                    const CreativeAdInfo& creative_ad) {
  const int count = base::ranges::count_if(
      ad_events, [&creative_ad](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == ConfirmationType::kServed &&
               ad_event.creative_set_id == creative_ad.creative_set_id;
      });

  return count < creative_ad.total_max;
}

}  // namespace

TotalMaxExclusionRule::TotalMaxExclusionRule(AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

TotalMaxExclusionRule::~TotalMaxExclusionRule() = default;

std::string TotalMaxExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool TotalMaxExclusionRule::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s has exceeded the totalMax frequency cap",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

const std::string& TotalMaxExclusionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads
