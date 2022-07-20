/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_H_

namespace brave_ads {

class NotificationHelper {
 public:
  NotificationHelper(const NotificationHelper&) = delete;
  NotificationHelper& operator=(const NotificationHelper&) = delete;
  virtual ~NotificationHelper();

  static NotificationHelper* GetInstance();

  virtual bool CanShowNativeNotifications() const;
  virtual bool CanShowNativeNotificationsWhileBrowserIsBackgrounded() const;

  virtual bool ShowOnboardingNotification();

 protected:
  friend class NotificationHelperHolder;

  NotificationHelper();

  static NotificationHelper* GetInstanceImpl();
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_H_
