/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ad_permission_rules.h"

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

namespace brave_ads {

// static
bool NotificationAdPermissionRules::HasPermission() {
  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  if (!HasUserActivityPermission()) {
    return false;
  }

  if (!HasCatalogPermission()) {
    return false;
  }

  if (!HasAllowNotificationsPermission()) {
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

  if (!HasNotificationAdsPerDayPermission()) {
    return false;
  }

  if (!HasNotificationAdsPerHourPermission()) {
    return false;
  }

  return HasNotificationAdMinimumWaitTimePermission();
}

}  // namespace brave_ads
