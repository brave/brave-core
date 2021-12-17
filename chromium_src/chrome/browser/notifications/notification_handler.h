/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_HANDLER_H_

#define NotificationHandler NotificationHandler_ChromiumImpl
#include "src/chrome/browser/notifications/notification_handler.h"
#undef NotificationHandler

#include "base/check.h"

class NotificationHandler : public NotificationHandler_ChromiumImpl {
 public:
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.notifications
  // GENERATED_JAVA_CLASS_NAME_OVERRIDE: NotificationType
  enum class Type {
    WEB_PERSISTENT = 0,
    WEB_NON_PERSISTENT = 1,
    EXTENSION = 2,
    SEND_TAB_TO_SELF = 3,
    TRANSIENT = 4,
    PERMISSION_REQUEST = 5,
    SHARING = 6,
    ANNOUNCEMENT = 7,
    NEARBY_SHARE = 8,
    BRAVE_ADS = 9,
    NOTIFICATIONS_MUTED = 10,
    TAILORED_SECURITY_CONSENTED = 11,
    MAX = TAILORED_SECURITY_CONSENTED,
  };

  // Make sure we know if the original enum gets changed.
  NotificationHandler() {
    DCHECK(static_cast<int>(NotificationHandler_ChromiumImpl::Type::MAX) + 1 ==
           static_cast<int>(Type::TAILORED_SECURITY_CONSENTED));
  }
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_HANDLER_H_
