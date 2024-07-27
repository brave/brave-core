/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule.h"

#include <utility>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource_info.h"

namespace brave_ads {

AntiTargetingExclusionRule::AntiTargetingExclusionRule(
    const AntiTargetingResource& resource,
    SiteHistoryList site_history)
    : resource_(resource), site_history_(std::move(site_history)) {}

AntiTargetingExclusionRule::~AntiTargetingExclusionRule() = default;

std::string AntiTargetingExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

base::expected<void, std::string> AntiTargetingExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(creative_ad)) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "creativeSetId $1 excluded due to visiting an anti-targeted site",
        {creative_ad.creative_set_id}, nullptr));
  }

  return base::ok();
}

///////////////////////////////////////////////////////////////////////////////

bool AntiTargetingExclusionRule::DoesRespectCap(
    const CreativeAdInfo& creative_ad) const {
  if (site_history_.empty()) {
    return true;
  }

  const AntiTargetingSiteList sites =
      resource_->GetSites(creative_ad.creative_set_id);
  if (sites.empty()) {
    return true;
  }

  return !HasVisitedAntiTargetedSites(site_history_, sites);
}

}  // namespace brave_ads
