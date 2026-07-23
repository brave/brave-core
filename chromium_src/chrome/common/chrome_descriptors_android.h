/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_COMMON_CHROME_DESCRIPTORS_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_CHROME_COMMON_CHROME_DESCRIPTORS_ANDROID_H_

#include <chrome/common/chrome_descriptors_android.h>  // IWYU pragma: export

enum {
  // File descriptor for brave_resources.pak, passed from the browser process
  // (which opens it from the APK via JNI) to child processes. Native-only
  // (javaless) renderers have no JVM and cannot open the APK asset themselves,
  // so they load the pak from this descriptor instead. Placed after Chrome's
  // last Android descriptor to avoid colliding with the upstream enum.
  kBraveResourcesPakDescriptor = kAndroidMinidumpDescriptor + 1,
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_COMMON_CHROME_DESCRIPTORS_ANDROID_H_
