/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule_util.h"

#include <algorithm>

#include "brave/components/brave_ads/core/public/common/url/url_util.h"

namespace brave_ads {

bool HasVisitedAntiTargetedSites(const SiteHistoryList& site_history,
                                 const AntiTargetingSiteList& sites) {
  return std::ranges::find_first_of(site_history, sites, SameDomainOrHost) !=
         site_history.cend();
}

}  // namespace brave_ads
