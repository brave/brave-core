/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/eligible_ads/exclusion_rules/per_week_exclusion_rule.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"

namespace ads {

PerWeekExclusionRule::PerWeekExclusionRule(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

PerWeekExclusionRule::~PerWeekExclusionRule() = default;

std::string PerWeekExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool PerWeekExclusionRule::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s has exceeded the perWeek frequency cap",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string PerWeekExclusionRule::GetLastMessage() const {
  return last_message_;
}

bool PerWeekExclusionRule::DoesRespectCap(const AdEventList& ad_events,
                                          const CreativeAdInfo& creative_ad) {
  if (creative_ad.per_week == 0) {
    // Always respect cap if set to 0
    return true;
  }

  return DoesRespectCreativeSetCap(creative_ad, ad_events,
                                   ConfirmationType::kServed, base::Days(7),
                                   creative_ad.per_week);
}

}  // namespace ads
