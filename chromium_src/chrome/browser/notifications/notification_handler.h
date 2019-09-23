/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_HANDLER_H_

#define NotificationHandler NotificationHandler_ChromiumImpl
#include "../../../../../../chrome/browser/notifications/notification_handler.h"
#undef NotificationHandler

class NotificationHandler : public NotificationHandler_ChromiumImpl {
 public:
  // If you add to the enum make sure the DCHECK in the constructor below is
  // updated.
  enum class Type {
    WEB_PERSISTENT = 0,
    WEB_NON_PERSISTENT = 1,
    EXTENSION = 2,
    SEND_TAB_TO_SELF = 3,
    TRANSIENT = 4,
    PERMISSION_REQUEST = 5,
    SHARING = 6,
    BRAVE_ADS = 7,
    MAX = BRAVE_ADS,
  };

  // Make sure we know if the original enum gets changed.
  NotificationHandler() {
    DCHECK(static_cast<int>(NotificationHandler_ChromiumImpl::Type::MAX) + 1 ==
           static_cast<int>(Type::MAX));
  }
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_HANDLER_H_
