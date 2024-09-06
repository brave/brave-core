/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/creative_instance_exclusion_rule.h"

#include <utility>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

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
  if (!DoesRespectCreativeCap(
          creative_ad, ad_events_, mojom::ConfirmationType::kServedImpression,
          kShouldExcludeAdIfCreativeInstanceWithinTimeWindow.Get(),
          kShouldExcludeAdIfCreativeInstanceExceedsPerHourCap.Get())) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "creativeInstanceId $1 has exceeded the creative instance frequency "
        "cap",
        {creative_ad.creative_instance_id}, nullptr));
  }

  return base::ok();
}

}  // namespace brave_ads
