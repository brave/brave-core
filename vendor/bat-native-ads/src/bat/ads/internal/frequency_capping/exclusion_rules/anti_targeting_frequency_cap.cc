/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/anti_targeting_frequency_cap.h"

#include <algorithm>
#include <memory>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/url_util.h"

namespace ads {

namespace {

bool HasVisitedSiteOnAntiTargetingList(
    const BrowsingHistoryList browsing_history,
    const resource::AntiTargetingList anti_targeting_sites) {
  const auto iter = std::find_first_of(
      anti_targeting_sites.cbegin(), anti_targeting_sites.cend(),
      browsing_history.cbegin(), browsing_history.cend(), SameDomainOrHost);
  if (iter == anti_targeting_sites.end()) {
    return false;
  }

  return true;
}

}  // namespace

AntiTargetingFrequencyCap::AntiTargetingFrequencyCap(
    resource::AntiTargeting* anti_targeting_resource,
    const BrowsingHistoryList& browsing_history)
    : browsing_history_(browsing_history) {
  anti_targeting_ = anti_targeting_resource->get();
}

AntiTargetingFrequencyCap::~AntiTargetingFrequencyCap() = default;

std::string AntiTargetingFrequencyCap::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool AntiTargetingFrequencyCap::ShouldExclude(
    const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s excluded due to visiting an anti-targeted site",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string AntiTargetingFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool AntiTargetingFrequencyCap::DoesRespectCap(
    const CreativeAdInfo& creative_ad) const {
  if (browsing_history_.empty()) {
    return true;
  }

  const auto iter = anti_targeting_.sites.find(creative_ad.creative_set_id);
  if (iter == anti_targeting_.sites.end()) {
    // Always respect if creative set has no anti-targeting sites
    return true;
  }

  return !HasVisitedSiteOnAntiTargetingList(browsing_history_, iter->second);
}

}  // namespace ads
