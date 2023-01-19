/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule.h"

#include <utility>

#include "base/ranges/algorithm.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/common/url/url_util.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

namespace ads {

namespace {

bool HasVisitedSiteOnAntiTargetingList(
    const BrowsingHistoryList& browsing_history,
    const resource::AntiTargetingSiteList& anti_targeting_sites) {
  const auto iter = base::ranges::find_first_of(
      anti_targeting_sites, browsing_history, SameDomainOrHost);
  return iter != anti_targeting_sites.cend();
}

}  // namespace

AntiTargetingExclusionRule::AntiTargetingExclusionRule(
    resource::AntiTargeting* anti_targeting_resource,
    BrowsingHistoryList browsing_history)
    : browsing_history_(std::move(browsing_history)) {
  anti_targeting_ = anti_targeting_resource->get();
}

AntiTargetingExclusionRule::~AntiTargetingExclusionRule() = default;

std::string AntiTargetingExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool AntiTargetingExclusionRule::ShouldExclude(
    const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s excluded due to visiting an anti-targeted site",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

const std::string& AntiTargetingExclusionRule::GetLastMessage() const {
  return last_message_;
}

bool AntiTargetingExclusionRule::DoesRespectCap(
    const CreativeAdInfo& creative_ad) const {
  if (browsing_history_.empty()) {
    return true;
  }

  const auto iter = anti_targeting_.sites.find(creative_ad.creative_set_id);
  if (iter == anti_targeting_.sites.cend()) {
    // Always respect if creative set has no anti-targeting sites
    return true;
  }

  return !HasVisitedSiteOnAntiTargetingList(browsing_history_, iter->second);
}

}  // namespace ads
