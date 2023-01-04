/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/per_hour_exclusion_rule.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"

namespace ads {

namespace {

constexpr int kPerHourCap = 1;

bool DoesRespectCap(const AdEventList& ad_events,
                    const CreativeAdInfo& creative_ad) {
  return DoesRespectCreativeCap(creative_ad, ad_events,
                                ConfirmationType::kServed, base::Hours(1),
                                kPerHourCap);
}

}  // namespace

PerHourExclusionRule::PerHourExclusionRule(AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

PerHourExclusionRule::~PerHourExclusionRule() = default;

std::string PerHourExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_instance_id;
}

bool PerHourExclusionRule::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeInstanceId %s has exceeded the perHour frequency cap",
        creative_ad.creative_instance_id.c_str());

    return true;
  }

  return false;
}

const std::string& PerHourExclusionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads
