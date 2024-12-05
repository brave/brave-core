/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_MOCK_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_MOCK_H_

#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper_impl.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class NotificationHelperImplMock : public NotificationHelperImpl {
 public:
  NotificationHelperImplMock();

  NotificationHelperImplMock(const NotificationHelperImplMock&) = delete;
  NotificationHelperImplMock& operator=(const NotificationHelperImplMock&) =
      delete;

  ~NotificationHelperImplMock() override;

  MOCK_METHOD(bool, CanShowNotifications, ());
  MOCK_METHOD(bool,
              CanShowSystemNotificationsWhileBrowserIsBackgrounded,
              (),
              (const));

  MOCK_METHOD(bool, ShowOnboardingNotification, ());
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_MOCK_H_
