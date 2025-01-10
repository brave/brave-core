/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/search_result_ads/search_result_ad_permission_rules.h"

#include <vector>

#include "base/trace_event/trace_event.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/ads_per_day_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/ads_per_hour_permission_rule.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_util.h"
#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_feature.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

// static
bool SearchResultAdPermissionRules::HasPermission(
    const AdEventList& ad_events) {
  TRACE_EVENT(kTraceEventCategory,
              "SearchResultAdPermissionRules::HasPermission");

  if (!UserHasJoinedBraveRewards()) {
    // If the user has not joined Brave Rewards, always grant permission.
    return true;
  }

  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  const std::vector<base::Time> history = ToHistory(ad_events);

  if (!HasAdsPerDayPermission(history,
                              /*cap=*/kMaximumSearchResultAdsPerDay.Get())) {
    return false;
  }

  if (!HasAdsPerHourPermission(history,
                               /*cap=*/kMaximumSearchResultAdsPerHour.Get())) {
    return false;
  }

  return true;
}

}  // namespace brave_ads
