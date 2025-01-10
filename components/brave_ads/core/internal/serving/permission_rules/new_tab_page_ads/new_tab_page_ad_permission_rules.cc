/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/new_tab_page_ads/new_tab_page_ad_permission_rules.h"

#include <vector>

#include "base/trace_event/trace_event.h"
#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/ads_per_day_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/ads_per_hour_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/catalog_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/minimum_wait_time_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/user_activity_permission_rule.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_util.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

// static
bool NewTabPageAdPermissionRules::HasPermission(const AdEventList& ad_events) {
  TRACE_EVENT(kTraceEventCategory,
              "NewTabPageAdPermissionRules::HasPermission");

  if (!UserHasJoinedBraveRewards()) {
    // If the user has not joined Brave Rewards, always grant permission.
    return true;
  }

  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  if (!HasUserActivityPermission()) {
    return false;
  }

  if (!HasCatalogPermission()) {
    return false;
  }

  const std::vector<base::Time> history = ToHistory(ad_events);

  if (!HasAdsPerDayPermission(history,
                              /*cap=*/kMaximumNewTabPageAdsPerDay.Get())) {
    return false;
  }

  if (!HasAdsPerHourPermission(history,
                               /*cap=*/kMaximumNewTabPageAdsPerHour.Get())) {
    return false;
  }

  if (!HasMinimumWaitTimePermission(
          history,
          /*time_constraint=*/kNewTabPageAdMinimumWaitTime.Get())) {
    return false;
  }

  return true;
}

}  // namespace brave_ads
