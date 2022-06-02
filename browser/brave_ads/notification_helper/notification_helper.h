/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_H_

namespace brave_ads {

class NotificationHelper {
 public:
  virtual ~NotificationHelper();

  NotificationHelper(const NotificationHelper&) = delete;
  NotificationHelper& operator=(const NotificationHelper&) = delete;

  static NotificationHelper* GetInstance();

  virtual bool CanShowNativeNotifications();

  virtual bool CanShowBackgroundNotifications() const;

  virtual bool ShowMyFirstNotificationAd();

 protected:
  friend class NotificationHelperHolder;

  NotificationHelper();

  static NotificationHelper* GetInstanceImpl();
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_H_
