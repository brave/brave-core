/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_H_

#include <memory>

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
}  // namespace base

namespace brave_ads {

class NotificationHelperImpl;

class NotificationHelper final {
 public:
  static NotificationHelper* GetInstance();

  bool CanShowNativeNotifications();
  bool CanShowNativeNotificationsWhileBrowserIsBackgrounded() const;

  bool ShowOnboardingNotification();

 private:
  friend struct base::DefaultSingletonTraits<NotificationHelper>;

  NotificationHelper();
  ~NotificationHelper();

  std::unique_ptr<NotificationHelperImpl> impl_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_H_
