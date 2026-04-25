/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"

#include <algorithm>

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"

namespace brave_ads {

namespace {

bool DoesCreativeAdTargetSubdivision(const CreativeAdInfo& creative_ad) {
  const auto iter = std::ranges::find_if(
      creative_ad.geo_targets, [](const std::string& geo_target) {
        return std::count(geo_target.cbegin(), geo_target.cend(), '-') == 1;
      });

  return iter != creative_ad.geo_targets.cend();
}

bool DoesCreativeAdTargetSubdivision(const CreativeAdInfo& creative_ad,
                                     const std::string& subdivision) {
  return creative_ad.geo_targets.contains(subdivision) ||
         creative_ad.geo_targets.contains(
             GetSubdivisionCountryCode(subdivision));
}

}  // namespace

SubdivisionTargetingExclusionRule::SubdivisionTargetingExclusionRule(
    const SubdivisionTargeting& subdivision_targeting)
    : subdivision_targeting_(subdivision_targeting) {}

SubdivisionTargetingExclusionRule::~SubdivisionTargetingExclusionRule() =
    default;

std::string SubdivisionTargetingExclusionRule::GetCacheKey(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool SubdivisionTargetingExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(creative_ad)) {
    BLOG(1, "creativeSetId "
                << creative_ad.creative_set_id
                << " excluded as not within the targeted subdivision");
    return false;
  }

  return true;
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

  return DoesCreativeAdTargetSubdivision(creative_ad, subdivision);
}

}  // namespace brave_ads
