/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/notification_helper/notification_helper_impl_mac.h"

#import <Cocoa/Cocoa.h>

// TODO(https://github.com/brave/brave-browser/issues/5541): Uncomment below
// code when notification_platform_bridge_mac.mm has been updated to use
// UNUserNotificationCenter
// #import <UserNotifications/UserNotifications.h>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/mac/mac_util.h"
#include "chrome/common/chrome_features.h"

namespace brave_ads {

namespace {

bool IsAuthorized() {
  // TODO(https://github.com/brave/brave-browser/issues/5541): Uncomment below
  // code when notification_platform_bridge_mac.mm has been updated to use
  // UNUserNotificationCenter
  return true;

  // #if !defined(OFFICIAL_BUILD)
  //   VLOG(1) << "Unable to detect the status of native notifications on non"
  //       " official builds as the app is not code signed";
  //   return true;
  // #else
  //   // TODO(https://openradar.appspot.com/27768556): We must mock this
  //   function
  //   // using NotificationHelperImplMock as a workaround to
  //   UNUserNotificationCenter
  //   // throwing an exception during tests

  //   if (@available(macOS 10.14, *)) {
  //     __block bool is_authorized = false;

  //     dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);

  //     UNUserNotificationCenter *notificationCenter =
  //         [UNUserNotificationCenter currentNotificationCenter];

  //     [notificationCenter
  //     requestAuthorizationWithOptions:UNAuthorizationOptionAlert
  //         completionHandler:^(BOOL granted, NSError * _Nullable error) {
  //       if (granted) {
  //         VLOG(1) << "User granted authorization to show notifications";
  //       } else {
  //         VLOG(1) << "User denied authorization to show notifications";
  //       }

  //       is_authorized = granted;

  //       dispatch_semaphore_signal(semaphore);
  //     }];

  //     dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
  //     dispatch_release(semaphore);

  //     if (!is_authorized) {
  //       VLOG(1) << "User is not authorized to show notifications";
  //     }

  //     return is_authorized;
  //   }

  //   return true;
  // #endif
}

bool IsEnabled() {
  // TODO(https://github.com/brave/brave-browser/issues/5541): Uncomment below
  // code when notification_platform_bridge_mac.mm has been updated to use
  // UNUserNotificationCenter
  return true;

  // #if !defined(OFFICIAL_BUILD)
  //   VLOG(1) << "Unable to detect the status of native notifications on non"
  //       " official builds as the app is not code signed";
  //   return true;
  // #else
  //   // TODO(https://openradar.appspot.com/27768556): We must mock this
  //   function
  //   // using NotificationHelperImplMock as a workaround to
  //   UNUserNotificationCenter
  //   // throwing an exception during tests

  //   if (@available(macOS 10.14, *)) {
  //     __block bool is_authorized = false;

  //     dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);

  //     UNUserNotificationCenter *notificationCenter =
  //         [UNUserNotificationCenter currentNotificationCenter];

  //     [notificationCenter getNotificationSettingsWithCompletionHandler:
  //         ^(UNNotificationSettings * _Nonnull settings) {
  //       switch (settings.authorizationStatus) {
  //         case UNAuthorizationStatusDenied: {
  //           VLOG(1) << "Notification authorization status denied";
  //           is_authorized = false;
  //           break;
  //         }

  //         case UNAuthorizationStatusNotDetermined: {
  //           VLOG(1) << "Notification authorization status not determined";
  //           is_authorized = true;
  //           break;
  //         }

  //         case UNAuthorizationStatusAuthorized: {
  //           VLOG(1) << "Notification authorization status authorized";
  //           is_authorized = true;
  //           break;
  //         }

  //         case UNAuthorizationStatusProvisional: {
  //           VLOG(1) << "Notification authorization status provisional";
  //           is_authorized = true;
  //           break;
  //         }
  //       }

  //       dispatch_semaphore_signal(semaphore);
  //     }];

  //     dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
  //     dispatch_release(semaphore);

  //     if (!is_authorized) {
  //       VLOG(1) << "Notifications not authorized";
  //     }

  //     return is_authorized;
  //   }

  //   return true;
  // #endif
}

}  // namespace

NotificationHelperImplMac::NotificationHelperImplMac() = default;

NotificationHelperImplMac::~NotificationHelperImplMac() = default;

bool NotificationHelperImplMac::CanShowNotifications() {
  if (!base::FeatureList::IsEnabled(::features::kNativeNotifications)) {
    VLOG(1) << "Native notifications feature is disabled";
    return false;
  }

  if (!IsAuthorized()) {
    return false;
  }

  if (!IsEnabled()) {
    return false;
  }

  return true;
}

bool NotificationHelperImplMac::
    CanShowSystemNotificationsWhileBrowserIsBackgrounded() const {
  return true;
}

bool NotificationHelperImplMac::ShowOnboardingNotification() {
  return false;
}

}  // namespace brave_ads
