/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NOTIFICATIONS_BRAVE_NOTIFICATION_PLATFORM_BRIDGE_H_
#define BRAVE_BROWSER_NOTIFICATIONS_BRAVE_NOTIFICATION_PLATFORM_BRIDGE_H_

#include <memory>

#include "base/macros.h"

class NotificationPlatformBridge;

class BraveNotificationPlatformBridge {
 public:
  static std::unique_ptr<NotificationPlatformBridge> Create();

 private:
  BraveNotificationPlatformBridge() = default;
  ~BraveNotificationPlatformBridge() = default;

  DISALLOW_COPY_AND_ASSIGN(BraveNotificationPlatformBridge);
};

#endif  // BRAVE_BROWSER_NOTIFICATIONS_BRAVE_NOTIFICATION_PLATFORM_BRIDGE_H_
