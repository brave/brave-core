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
  enum class Type {
    WEB_PERSISTENT = 0,
    WEB_NON_PERSISTENT = 1,
    EXTENSION = 2,
    SEND_TAB_TO_SELF = 3,
    TRANSIENT = 4,
    BRAVE_ADS = 5,
    MAX = BRAVE_ADS,
  };
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_HANDLER_H_
