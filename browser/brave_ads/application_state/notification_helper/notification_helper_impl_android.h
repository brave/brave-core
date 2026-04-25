/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_ANDROID_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_ANDROID_H_

#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper_impl.h"

namespace brave_ads {

class NotificationHelperImplAndroid final : public NotificationHelperImpl {
 public:
  NotificationHelperImplAndroid(const NotificationHelperImplAndroid&) = delete;
  NotificationHelperImplAndroid& operator=(
      const NotificationHelperImplAndroid&) = delete;

  ~NotificationHelperImplAndroid() override;

 protected:
  friend class NotificationHelper;

  NotificationHelperImplAndroid();

 private:
  // NotificationHelperLinux:
  bool CanShowNotifications() override;
  bool CanShowSystemNotificationsWhileBrowserIsBackgrounded() const override;

  bool ShowOnboardingNotification() override;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_ANDROID_H_
