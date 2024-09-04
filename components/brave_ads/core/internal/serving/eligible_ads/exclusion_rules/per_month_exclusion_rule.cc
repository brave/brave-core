/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/per_month_exclusion_rule.h"

#include <utility>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

PerMonthExclusionRule::PerMonthExclusionRule(AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

PerMonthExclusionRule::~PerMonthExclusionRule() = default;

std::string PerMonthExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

base::expected<void, std::string> PerMonthExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCreativeSetCap(
          creative_ad, ad_events_, mojom::ConfirmationType::kServedImpression,
          /*time_constraint=*/base::Days(28), creative_ad.per_month)) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "creativeSetId $1 has exceeded the perMonth frequency cap",
        {creative_ad.creative_set_id}, nullptr));
  }

  return base::ok();
}

}  // namespace brave_ads
