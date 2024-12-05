/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_H_

namespace brave_ads {

class NotificationHelperImpl {
 public:
  NotificationHelperImpl(const NotificationHelperImpl&) = delete;
  NotificationHelperImpl& operator=(const NotificationHelperImpl&) = delete;

  virtual ~NotificationHelperImpl();

  virtual bool CanShowNotifications();
  virtual bool CanShowSystemNotificationsWhileBrowserIsBackgrounded() const;

  virtual bool ShowOnboardingNotification();

 protected:
  friend class NotificationHelper;

  NotificationHelperImpl();
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_H_
