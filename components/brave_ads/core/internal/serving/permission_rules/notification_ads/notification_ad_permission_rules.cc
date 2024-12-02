/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ad_permission_rules.h"

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/ads_per_day_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/ads_per_hour_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/browser_is_active_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/catalog_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/do_not_disturb_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/full_screen_mode_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/media_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/minimum_wait_time_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/network_connection_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/can_show_notifications_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/user_activity_permission_rule.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_util.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"

namespace brave_ads {

// static
bool NotificationAdPermissionRules::HasPermission(
    const AdEventList& ad_events) {
  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  if (!HasUserActivityPermission()) {
    return false;
  }

  if (!HasCatalogPermission()) {
    return false;
  }

  if (!HasCanShowNotificationsPermission()) {
    return false;
  }

  if (!HasNetworkConnectionPermission()) {
    return false;
  }

  if (!HasFullScreenModePermission()) {
    return false;
  }

  if (!HasBrowserIsActivePermission()) {
    return false;
  }

  if (!HasDoNotDisturbPermission()) {
    return false;
  }

  if (!HasMediaPermission()) {
    return false;
  }

  const std::vector<base::Time> history = ToHistory(ad_events);

  if (!HasAdsPerDayPermission(history,
                              /*cap=*/kMaximumNotificationAdsPerDay.Get())) {
    return false;
  }

  if (!HasAdsPerHourPermission(history,
                               /*cap=*/GetMaximumNotificationAdsPerHour())) {
    return false;
  }

  return HasMinimumWaitTimePermission(
      history,
      /*time_constraint=*/base::Hours(1) / GetMaximumNotificationAdsPerHour());
}

}  // namespace brave_ads
