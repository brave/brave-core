/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/build/android/jni_headers/BraveNotificationPlatformBridge_jni.h"
#include "chrome/browser/notifications/notification_platform_bridge_android.h"
#include "chrome/android/chrome_jni_headers/NotificationPlatformBridge_jni.h"

namespace {

// Preventing unused Java_NotificationPlatformBridge_create error
class UnusedClass {
 private:
  void test() {
    Java_NotificationPlatformBridge_create(nullptr, 0ll);
  }
};

}  // namespace

#define Java_NotificationPlatformBridge_create \
  Java_BraveNotificationPlatformBridge_create
#include "src/chrome/browser/notifications/notification_platform_bridge_android.cc"
#undef Java_NotificationPlatformBridge_create
