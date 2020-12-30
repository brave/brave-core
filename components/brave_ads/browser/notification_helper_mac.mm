/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Cocoa/Cocoa.h>

// TODO(https://github.com/brave/brave-browser/issues/5541): Uncomment below
// code when notification_platform_bridge_mac.mm has been updated to use
// UNUserNotificationCenter
// #import <UserNotifications/UserNotifications.h>

#include "brave/components/brave_ads/browser/notification_helper_mac.h"

#include "base/logging.h"
#include "chrome/browser/fullscreen.h"

namespace brave_ads {

NotificationHelperMac::NotificationHelperMac() = default;

NotificationHelperMac::~NotificationHelperMac() = default;

bool NotificationHelperMac::ShouldShowNotifications() {
  if (IsFullScreenMode()) {
    LOG(WARNING) << "Notification not made: Full screen mode";
    return false;
  }

  return true;
}

bool NotificationHelperMac::ShowMyFirstAdNotification() {
  return false;
}

NotificationHelperMac* NotificationHelperMac::GetInstanceImpl() {
  return base::Singleton<NotificationHelperMac>::get();
}

NotificationHelper* NotificationHelper::GetInstanceImpl() {
  return NotificationHelperMac::GetInstanceImpl();
}

}  // namespace brave_ads
