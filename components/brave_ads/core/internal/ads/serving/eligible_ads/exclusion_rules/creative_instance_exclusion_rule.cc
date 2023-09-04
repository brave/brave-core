/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/creative_instance_exclusion_rule.h"

#include <utility>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/public/confirmation_type.h"

namespace brave_ads {

namespace {

constexpr int kPerHourCap = 1;

bool DoesRespectCap(const AdEventList& ad_events,
                    const CreativeAdInfo& creative_ad) {
  return DoesRespectCreativeCap(creative_ad, ad_events,
                                ConfirmationType::kServed, base::Hours(1),
                                kPerHourCap);
}

}  // namespace

CreativeInstanceExclusionRule::CreativeInstanceExclusionRule(
    AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

CreativeInstanceExclusionRule::~CreativeInstanceExclusionRule() = default;

std::string CreativeInstanceExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_instance_id;
}

base::expected<void, std::string> CreativeInstanceExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "creativeInstanceId $1 has exceeded the creative instance frequency "
        "cap",
        {creative_ad.creative_instance_id}, nullptr));
  }

  return base::ok();
}

}  // namespace brave_ads
