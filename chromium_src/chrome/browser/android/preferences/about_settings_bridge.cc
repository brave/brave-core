/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/android/apk_info.h"
#include "base/android/jni_string.h"
#include "brave/components/version_info/version_info.h"
#include "chrome/android/chrome_jni_headers/AboutSettingsBridge_jni.h"

#define JNI_AboutSettingsBridge_GetApplicationVersion \
  JNI_AboutSettingsBridge_GetApplicationVersion_ChromiumImpl
// Suppress DEFINE_JNI in included file - we call it ourselves at the end
#pragma push_macro("DEFINE_JNI")
#undef DEFINE_JNI
#define DEFINE_JNI(...)
#include <chrome/browser/android/preferences/about_settings_bridge.cc>
#undef DEFINE_JNI
#pragma pop_macro("DEFINE_JNI")
#undef JNI_AboutSettingsBridge_GetApplicationVersion

static std::string JNI_AboutSettingsBridge_GetApplicationVersion(JNIEnv* env) {
  JNI_AboutSettingsBridge_GetApplicationVersion_ChromiumImpl(env);

  std::string application(base::android::apk_info::host_package_label());
  application.append(" ");
  application.append(
      version_info::GetBraveVersionWithoutChromiumMajorVersion());
  application.append(", Chromium ");
  application.append(version_info::GetBraveChromiumVersionNumber());

  return application;
}

DEFINE_JNI(AboutSettingsBridge)
