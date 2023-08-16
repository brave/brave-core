/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"

#include <vector>

#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/locale/subdivision_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"

namespace brave_ads {

namespace {

bool DoesCreativeAdSupportSubdivision(const CreativeAdInfo& creative_ad,
                                      const std::string& subdivision) {
  return creative_ad.geo_targets.find(subdivision) !=
             creative_ad.geo_targets.cend() ||
         creative_ad.geo_targets.find(GetSubdivisionCountryCode(subdivision)) !=
             creative_ad.geo_targets.cend();
}

bool DoesCreativeAdTargetSubdivision(const CreativeAdInfo& creative_ad) {
  const auto iter = base::ranges::find_if(
      creative_ad.geo_targets, [](const std::string& geo_target) {
        const std::vector<std::string> components = base::SplitString(
            geo_target, "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

        return components.size() == 2;
      });

  return iter != creative_ad.geo_targets.cend();
}

}  // namespace

SubdivisionTargetingExclusionRule::SubdivisionTargetingExclusionRule(
    const SubdivisionTargeting& subdivision_targeting)
    : subdivision_targeting_(subdivision_targeting) {}

SubdivisionTargetingExclusionRule::~SubdivisionTargetingExclusionRule() =
    default;

std::string SubdivisionTargetingExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

base::expected<void, std::string>
SubdivisionTargetingExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(creative_ad)) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "creativeSetId $1 excluded as not within the targeted subdivision",
        {creative_ad.creative_set_id}, nullptr));
  }

  return base::ok();
}

///////////////////////////////////////////////////////////////////////////////

bool SubdivisionTargetingExclusionRule::DoesRespectCap(
    const CreativeAdInfo& creative_ad) const {
  if (!SubdivisionTargeting::ShouldAllow() ||
      subdivision_targeting_->IsDisabled()) {
    return !DoesCreativeAdTargetSubdivision(creative_ad);
  }

  const std::string& subdivision = subdivision_targeting_->GetSubdivision();
  if (subdivision.empty()) {
    return false;
  }

  return DoesCreativeAdSupportSubdivision(creative_ad, subdivision);
}

}  // namespace brave_ads
