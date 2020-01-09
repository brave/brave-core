/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/version_info.h"
#include "chrome/android/chrome_jni_headers/AboutSettingsBridge_jni.h"

#define JNI_AboutSettingsBridge_GetApplicationVersion \
  JNI_AboutSettingsBridge_GetApplicationVersion_ChromiumImpl
#include "../../../../../chrome/browser/android/preferences/about_settings_bridge.cc"
#undef JNI_AboutSettingsBridge_GetApplicationVersion

static ScopedJavaLocalRef<jstring>
JNI_AboutSettingsBridge_GetApplicationVersion(JNIEnv* env) {
  JNI_AboutSettingsBridge_GetApplicationVersion_ChromiumImpl(env);

  base::android::BuildInfo* android_build_info =
      base::android::BuildInfo::GetInstance();
  std::string application(android_build_info->host_package_label());
  application.append(" ");
  application.append(android_build_info->package_version_name());
  application.append(", Chromium ");
  application.append(version_info::GetBraveChromiumVersionNumber());

  return ConvertUTF8ToJavaString(env, application);
}
