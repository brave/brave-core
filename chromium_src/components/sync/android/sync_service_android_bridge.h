/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ANDROID_SYNC_SERVICE_ANDROID_BRIDGE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ANDROID_SYNC_SERVICE_ANDROID_BRIDGE_H_

#define KeepAccountSettingsPrefsOnlyForUsers                                   \
  KeepAccountSettingsPrefsOnlyForUsers_Unused(                                 \
      JNIEnv* env, const base::android::JavaParamRef<jobjectArray>& gaia_ids); \
  void KeepAccountSettingsPrefsOnlyForUsers

#include "src/components/sync/android/sync_service_android_bridge.h"  // IWYU pragma: export

#undef KeepAccountSettingsPrefsOnlyForUsers

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ANDROID_SYNC_SERVICE_ANDROID_BRIDGE_H_
