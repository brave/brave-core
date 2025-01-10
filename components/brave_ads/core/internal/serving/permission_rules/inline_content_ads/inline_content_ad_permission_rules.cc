/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/inline_content_ads/inline_content_ad_permission_rules.h"

#include <vector>

#include "base/trace_event/trace_event.h"
#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/ads_per_day_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/ads_per_hour_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/catalog_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/user_activity_permission_rule.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_util.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

// static
bool InlineContentAdPermissionRules::HasPermission(
    const AdEventList& ad_events) {
  TRACE_EVENT(kTraceEventCategory,
              "InlineContentAdPermissionRules::HasPermission");

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
                              /*cap=*/kMaximumInlineContentAdsPerDay.Get())) {
    return false;
  }

  return HasAdsPerHourPermission(history,
                                 /*cap=*/kMaximumInlineContentAdsPerHour.Get());
}

}  // namespace brave_ads
