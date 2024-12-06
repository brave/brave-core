/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_ANDROID_H_

#define OnNotificationProcessed                              \
  Unused() {}                                                \
  friend class BraveNotificationPlatformBridgeHelperAndroid; \
  void OnNotificationProcessed

#include "src/chrome/browser/notifications/notification_platform_bridge_android.h"  // IWYU pragma: export

#undef OnNotificationProcessed

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_ANDROID_H_
