/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule_util.h"

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"

namespace brave_ads {

bool HasVisitedAntiTargetedSites(const BrowsingHistoryList& browsing_history,
                                 const AntiTargetingSiteList& sites) {
  return base::ranges::find_first_of(browsing_history, sites,
                                     SameDomainOrHost) !=
         browsing_history.cend();
}

}  // namespace brave_ads
