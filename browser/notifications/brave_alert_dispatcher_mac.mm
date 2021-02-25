/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/browser/notifications/brave_alert_dispatcher_mac.h"

#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/services/mac_notifications/public/cpp/notification_constants_mac.h"
#include "chrome/browser/ui/cocoa/notifications/notification_builder_mac.h"

@implementation BraveAlertDispatcherMac {
  base::scoped_nsobject<NSMutableArray> alerts_;
}

- (void)dispatchNotification:(NSDictionary *)data {
  base::scoped_nsobject<NotificationBuilder> builder(
      [[NotificationBuilder alloc] initWithDictionary:data]);

  NSUserNotification * toast = [builder buildUserNotification];
  [[NSUserNotificationCenter defaultUserNotificationCenter]
      deliverNotification: toast];
}

- (void)closeNotificationWithId:(NSString *)notificationId
                  withProfileId:(NSString *)profileId {
  NSUserNotificationCenter * notificationCenter =
      [NSUserNotificationCenter defaultUserNotificationCenter];
  for (NSUserNotification * candidate in
       [notificationCenter deliveredNotifications]) {
    NSString* candidateId = [candidate.userInfo
        objectForKey: notification_constants::kNotificationId];

    NSString* candidateProfileId = [candidate.userInfo
        objectForKey: notification_constants::kNotificationProfileId];

    if ([candidateId isEqualToString: notificationId] &&
        [profileId isEqualToString: candidateProfileId]) {
      [notificationCenter removeDeliveredNotification:candidate];
      break;
    }
  }
}

- (void)closeAllNotifications {
  [[NSUserNotificationCenter defaultUserNotificationCenter]
      removeAllDeliveredNotifications];
}

- (void)
getDisplayedAlertsForProfileId:(NSString *)profileId
                     incognito:(BOOL)incognito
            notificationCenter:(NSUserNotificationCenter*)notificationCenter
                      callback:(GetDisplayedNotificationsCallback)callback {
  std::set<std::string> displayedNotifications;
  for (NSUserNotification * toast in
      [notificationCenter deliveredNotifications]) {
    NSString * toastProfileId = [toast.userInfo
        objectForKey:notification_constants::kNotificationProfileId];
    if ([toastProfileId isEqualToString:profileId]) {
      displayedNotifications.insert(base::SysNSStringToUTF8([toast.userInfo
          objectForKey: notification_constants::kNotificationId]));
    }
  }

  std::move(callback).Run(std::move(displayedNotifications),
                          true /* supports_synchronization */);
}

@end
