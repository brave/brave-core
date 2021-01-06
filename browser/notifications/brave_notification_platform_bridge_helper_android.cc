/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/notifications/brave_notification_platform_bridge_helper_android.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "chrome/browser/notifications/notification_platform_bridge_android.h"

void BraveNotificationPlatformBridgeHelperAndroid::MaybeRegenerateNotification(
    const std::string& notification_id,
    const GURL& service_worker_scope) {
  NotificationPlatformBridgeAndroid* notification_platform_bridge =
      static_cast<NotificationPlatformBridgeAndroid*>(
          g_browser_process->notification_platform_bridge());
  const auto iterator =
      notification_platform_bridge->regenerated_notification_infos_.find(
          notification_id);
  if (iterator ==
      notification_platform_bridge->regenerated_notification_infos_.end()) {
    NotificationPlatformBridgeAndroid::RegeneratedNotificationInfo
        notification_info;
    notification_info.service_worker_scope = service_worker_scope;
    notification_platform_bridge
        ->regenerated_notification_infos_[notification_id] = notification_info;
  }
}
