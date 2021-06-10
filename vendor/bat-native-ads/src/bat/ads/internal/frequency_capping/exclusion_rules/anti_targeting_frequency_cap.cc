/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/anti_targeting_frequency_cap.h"

#include <algorithm>
#include <memory>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/url_util.h"

namespace ads {

namespace {

bool HasVisitedSiteOnAntiTargetingList(
    const BrowsingHistoryList browsing_history,
    const resource::AntiTargetingList anti_targeting_sites) {
  auto result = std::find_first_of(
      anti_targeting_sites.begin(), anti_targeting_sites.end(),
      browsing_history.begin(), browsing_history.end(), SameDomainOrHost);

  if (result != anti_targeting_sites.end()) {
    return true;
  }

  return false;
}

}  // namespace

AntiTargetingFrequencyCap::AntiTargetingFrequencyCap(
    resource::AntiTargeting* anti_targeting_resource,
    const BrowsingHistoryList& browsing_history)
    : anti_targeting_resource_(anti_targeting_resource),
      browsing_history_(browsing_history) {}

AntiTargetingFrequencyCap::~AntiTargetingFrequencyCap() = default;

bool AntiTargetingFrequencyCap::ShouldExclude(const CreativeAdInfo& ad) {
  if (!DoesRespectCap(ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s excluded as previously visited an anti-targeted site",
        ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string AntiTargetingFrequencyCap::get_last_message() const {
  return last_message_;
}

bool AntiTargetingFrequencyCap::DoesRespectCap(const CreativeAdInfo& ad) const {
  if (browsing_history_.empty()) {
    return true;
  }

  resource::AntiTargetingInfo anti_targeting = anti_targeting_resource_->get();
  const auto iter = anti_targeting.sites.find(ad.creative_set_id);
  if (iter == anti_targeting.sites.end()) {
    // Always respect if creative set has no anti-targeting sites
    return true;
  }

  return !HasVisitedSiteOnAntiTargetingList(browsing_history_, iter->second);
}

}  // namespace ads
