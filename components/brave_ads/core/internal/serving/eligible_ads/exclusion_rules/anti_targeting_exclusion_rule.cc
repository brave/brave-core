/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule.h"

#include <utility>

#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

bool HasVisitedSiteOnAntiTargetingList(
    const BrowsingHistoryList& browsing_history,
    const AntiTargetingSiteList& anti_targeting_sites) {
  const auto iter = base::ranges::find_first_of(
      anti_targeting_sites, browsing_history, SameDomainOrHost);
  return iter != anti_targeting_sites.cend();
}

}  // namespace

AntiTargetingExclusionRule::AntiTargetingExclusionRule(
    const AntiTargetingResource& resource,
    BrowsingHistoryList browsing_history)
    : resource_(resource), browsing_history_(std::move(browsing_history)) {}

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
  if (browsing_history_.empty()) {
    return true;
  }

  const absl::optional<AntiTargetingInfo>& anti_targeting = resource_->get();
  if (!anti_targeting) {
    return true;
  }

  const auto iter = anti_targeting->sites.find(creative_ad.creative_set_id);
  if (iter == anti_targeting->sites.cend()) {
    // Always respect if creative set has no anti-targeting sites.
    return true;
  }

  return !HasVisitedSiteOnAntiTargetingList(browsing_history_, iter->second);
}

}  // namespace brave_ads
