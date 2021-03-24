/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/browser/notifications/brave_alert_dispatcher_mac.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/cocoa/notifications/notification_builder_mac.h"
#include "chrome/services/mac_notifications/public/cpp/notification_constants_mac.h"

// clang-format off
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
                  profileId:(NSString *)profileId
                  incognito:(BOOL)incognito {
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

- (void)closeNotificationsWithProfileId:(NSString*)profileId
                              incognito:(BOOL)incognito {
  DCHECK(profileId);
  NSUserNotificationCenter * notificationCenter =
      [NSUserNotificationCenter defaultUserNotificationCenter];
  for (NSUserNotification * candidate in
       [notificationCenter deliveredNotifications]) {
    NSString* candidateProfileId = [candidate.userInfo
        objectForKey: notification_constants::kNotificationProfileId];

    bool candidateIncognito = [[candidate.userInfo
        objectForKey:notification_constants::kNotificationIncognito] boolValue];

    if ([profileId isEqualToString: candidateProfileId] &&
        incognito == candidateIncognito) {
      [notificationCenter removeDeliveredNotification:candidate];
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
                      callback:(GetDisplayedNotificationsCallback)callback {
  NSUserNotificationCenter * notificationCenter =
      [NSUserNotificationCenter defaultUserNotificationCenter];
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

- (void)getAllDisplayedAlertsWithCallback:
    (GetAllDisplayedNotificationsCallback)callback {
  NSUserNotificationCenter * notificationCenter =
      [NSUserNotificationCenter defaultUserNotificationCenter];

  std::vector<MacNotificationIdentifier> alertIds;
  for (NSUserNotification * toast in
      [notificationCenter deliveredNotifications]) {
    std::string notificationId = base::SysNSStringToUTF8(
        [toast.userInfo objectForKey:notification_constants::kNotificationId]);
    std::string profileId = base::SysNSStringToUTF8([toast.userInfo
         objectForKey:notification_constants::kNotificationProfileId]);
    bool incognito = [[toast.userInfo
        objectForKey:notification_constants::kNotificationIncognito] boolValue];

    alertIds.push_back(
        {std::move(notificationId), std::move(profileId), incognito});
  }

  // Create set from std::vector to avoid N^2 insertion runtime.
  base::flat_set<MacNotificationIdentifier> alertSet(std::move(alertIds));
  std::move(callback).Run(std::move(alertSet));
}

@end
// clang-format on
