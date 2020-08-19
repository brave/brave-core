/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_CPP_NOTIFICATION_TYPES_H_
#define BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_CPP_NOTIFICATION_TYPES_H_

namespace brave_custom_notification {

// Notification types. Note that the values in this enumeration are being
// recoded in a histogram, updates should not change the entries' values.
enum NotificationType {
  NOTIFICATION_TYPE_SIMPLE = 0,
  NOTIFICATION_TYPE_BASE_FORMAT = 1,
  NOTIFICATION_TYPE_IMAGE = 2,
  NOTIFICATION_TYPE_MULTIPLE = 3,
  NOTIFICATION_TYPE_PROGRESS = 4,  // Notification with progress bar.
  NOTIFICATION_TYPE_CUSTOM = 5,

  // Add new values before this line.
  NOTIFICATION_TYPE_LAST = NOTIFICATION_TYPE_CUSTOM
};

enum NotificationPriority {
  MIN_PRIORITY = -2,
  LOW_PRIORITY = -1,
  DEFAULT_PRIORITY = 0,
  // Priorities > |DEFAULT_PRIORITY| have the capability to wake the display up
  // if it was off.
  HIGH_PRIORITY = 1,
  MAX_PRIORITY = 2,

  // Top priority for system-level notifications.. This can't be set from
  // kPriorityKey, instead you have to call SetSystemPriority() of
  // Notification object.
  SYSTEM_PRIORITY = 3,
};

}  // namespace brave_custom_notification

#endif  // BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_PUBLIC_CPP_NOTIFICATION_TYPES_H_