/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/per_week_exclusion_rule.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

PerWeekExclusionRule::PerWeekExclusionRule(AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

PerWeekExclusionRule::~PerWeekExclusionRule() = default;

std::string PerWeekExclusionRule::GetCacheKey(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool PerWeekExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCreativeSetCap(
          creative_ad, ad_events_, mojom::ConfirmationType::kServedImpression,
          /*time_constraint=*/base::Days(7), creative_ad.per_week)) {
    BLOG(1, "creativeSetId " << creative_ad.creative_set_id
                             << " has exceeded the perWeek frequency cap");
    return false;
  }

  return true;
}

}  // namespace brave_ads
