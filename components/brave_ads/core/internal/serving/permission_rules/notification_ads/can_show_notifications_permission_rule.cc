/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/can_show_notifications_permission_rule.h"

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

bool HasCanShowNotificationsPermission() {
  if (GetAdsClient().CanShowNotificationAds()) {
    return true;
  }

  BLOG(2, "Notifications cannot be shown");
  return false;
}

}  // namespace brave_ads
