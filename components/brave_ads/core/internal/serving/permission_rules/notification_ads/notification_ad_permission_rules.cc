/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ad_permission_rules.h"

#include "brave/components/brave_ads/core/internal/serving/permission_rules/allow_notifications_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/browser_is_active_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/catalog_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/do_not_disturb_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/full_screen_mode_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/media_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/network_connection_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ads_minimum_wait_time_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ads_per_day_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ads_per_hour_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/user_activity_permission_rule.h"

namespace brave_ads {

// static
bool NotificationAdPermissionRules::HasPermission() {
  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  const UserActivityPermissionRule user_activity_permission_rule;
  if (!ShouldAllow(user_activity_permission_rule)) {
    return false;
  }

  const CatalogPermissionRule catalog_permission_rule;
  if (!ShouldAllow(catalog_permission_rule)) {
    return false;
  }

  const AllowNotificationsPermissionRule allow_notifications_permission_rule;
  if (!ShouldAllow(allow_notifications_permission_rule)) {
    return false;
  }

  const NetworkConnectionPermissionRule network_connection_permission_rule;
  if (!ShouldAllow(network_connection_permission_rule)) {
    return false;
  }

  const FullScreenModePermissionRule full_screen_mode_permission_rule;
  if (!ShouldAllow(full_screen_mode_permission_rule)) {
    return false;
  }

  const BrowserIsActivePermissionRule browser_is_active_permission_rule;
  if (!ShouldAllow(browser_is_active_permission_rule)) {
    return false;
  }

  const DoNotDisturbPermissionRule do_not_disturb_permission_rule;
  if (!ShouldAllow(do_not_disturb_permission_rule)) {
    return false;
  }

  const MediaPermissionRule media_permission_rule;
  if (!ShouldAllow(media_permission_rule)) {
    return false;
  }

  const NotificationAdsPerDayPermissionRule ads_per_day_permission_rule;
  if (!ShouldAllow(ads_per_day_permission_rule)) {
    return false;
  }

  const NotificationAdsPerHourPermissionRule ads_per_hour_permission_rule;
  if (!ShouldAllow(ads_per_hour_permission_rule)) {
    return false;
  }

  const NotificationAdMinimumWaitTimePermissionRule
      minimum_wait_time_permission_rule;
  return ShouldAllow(minimum_wait_time_permission_rule);
}

}  // namespace brave_ads
