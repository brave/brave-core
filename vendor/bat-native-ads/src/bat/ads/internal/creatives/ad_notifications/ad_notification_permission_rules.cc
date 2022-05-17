/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/ad_notifications/ad_notification_permission_rules.h"

#include "bat/ads/internal/serving/permission_rules/ads_per_day_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/ads_per_hour_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/allow_notifications_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/browser_is_active_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/catalog_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/do_not_disturb_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/full_screen_mode_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/media_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/minimum_wait_time_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/network_connection_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/permission_rule_util.h"
#include "bat/ads/internal/serving/permission_rules/user_activity_permission_rule.h"

namespace ads {
namespace ad_notifications {
namespace frequency_capping {

PermissionRules::PermissionRules() : PermissionRulesBase() {}

PermissionRules::~PermissionRules() = default;

bool PermissionRules::HasPermission() const {
  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  UserActivityPermissionRule user_activity_permission_rule;
  if (!ShouldAllow(&user_activity_permission_rule)) {
    return false;
  }

  CatalogPermissionRule catalog_permission_rule;
  if (!ShouldAllow(&catalog_permission_rule)) {
    return false;
  }

  AllowNotificationsPermissionRule allow_notifications_permission_rule;
  if (!ShouldAllow(&allow_notifications_permission_rule)) {
    return false;
  }

  NetworkConnectionPermissionRule network_connection_permission_rule;
  if (!ShouldAllow(&network_connection_permission_rule)) {
    return false;
  }

  FullScreenModePermissionRule full_screen_mode_permission_rule;
  if (!ShouldAllow(&full_screen_mode_permission_rule)) {
    return false;
  }

  BrowserIsActivePermissionRule browser_is_active_permission_rule;
  if (!ShouldAllow(&browser_is_active_permission_rule)) {
    return false;
  }

  DoNotDisturbPermissionRule do_not_disturb_permission_rule;
  if (!ShouldAllow(&do_not_disturb_permission_rule)) {
    return false;
  }

  MediaPermissionRule media_permission_rule;
  if (!ShouldAllow(&media_permission_rule)) {
    return false;
  }

  AdsPerDayPermissionRule ads_per_day_permission_rule;
  if (!ShouldAllow(&ads_per_day_permission_rule)) {
    return false;
  }

  AdsPerHourPermissionRule ads_per_hour_permission_rule;
  if (!ShouldAllow(&ads_per_hour_permission_rule)) {
    return false;
  }

  MinimumWaitTimePermissionRule minimum_wait_time_permission_rule;
  if (!ShouldAllow(&minimum_wait_time_permission_rule)) {
    return false;
  }

  return true;
}

}  // namespace frequency_capping
}  // namespace ad_notifications
}  // namespace ads
