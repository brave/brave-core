/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_HOLDER_H_
#define BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_HOLDER_H_

#include <memory>

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
}  // namespace base

namespace brave_ads {

class NotificationHelper;

class NotificationHelperHolder final {
 public:
  static NotificationHelperHolder* GetInstance();

  NotificationHelper* GetNotificationHelper();

 private:
  friend struct base::DefaultSingletonTraits<NotificationHelperHolder>;

  NotificationHelperHolder();
  ~NotificationHelperHolder();

  std::unique_ptr<NotificationHelper> notification_helper_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_HOLDER_H_
