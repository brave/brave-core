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

  if (!ShouldAllowUserActivity()) {
    return false;
  }

  if (!ShouldAllowCatalog()) {
    return false;
  }

  if (!ShouldAllowAllowNotifications()) {
    return false;
  }

  if (!ShouldAllowNetworkConnection()) {
    return false;
  }

  if (!ShouldAllowFullScreenMode()) {
    return false;
  }

  if (!ShouldAllowBrowserIsActive()) {
    return false;
  }

  if (!ShouldAllowDoNotDisturb()) {
    return false;
  }

  if (!ShouldAllowMedia()) {
    return false;
  }

  if (!ShouldAllowNotificationAdsPerDay()) {
    return false;
  }

  if (!ShouldAllowNotificationAdsPerHour()) {
    return false;
  }

  return ShouldAllowNotificationAdMinimumWaitTime();
}

}  // namespace brave_ads
