/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_MOCK_H_
#define BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_MOCK_H_

#include "brave/browser/brave_ads/notification_helper/notification_helper_impl.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class NotificationHelperImplMock : public NotificationHelperImpl {
 public:
  NotificationHelperImplMock();
  NotificationHelperImplMock(const NotificationHelperImplMock&) = delete;
  NotificationHelperImplMock& operator=(const NotificationHelperImplMock&) =
      delete;
  ~NotificationHelperImplMock() override;

  MOCK_METHOD0(CanShowNativeNotifications, bool());
  MOCK_CONST_METHOD0(CanShowNativeNotificationsWhileBrowserIsBackgrounded,
                     bool());

  MOCK_METHOD0(ShowOnboardingNotification, bool());
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_MOCK_H_
