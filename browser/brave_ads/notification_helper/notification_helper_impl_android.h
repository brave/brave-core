/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_ANDROID_H_
#define BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_ANDROID_H_

#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_ads/notification_helper/notification_helper_impl.h"

namespace brave_ads {

class NotificationHelperImplAndroid
    : public NotificationHelperImpl,
      public base::SupportsWeakPtr<NotificationHelperImplAndroid> {
 public:
  NotificationHelperImplAndroid(const NotificationHelperImplAndroid&) = delete;
  NotificationHelperImplAndroid& operator=(
      const NotificationHelperImplAndroid&) = delete;

  NotificationHelperImplAndroid(
      NotificationHelperImplAndroid&& other) noexcept = delete;
  NotificationHelperImplAndroid& operator=(
      NotificationHelperImplAndroid&& other) noexcept = delete;

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

#endif  // BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_ANDROID_H_
