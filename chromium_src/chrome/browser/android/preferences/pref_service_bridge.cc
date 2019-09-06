/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/android/chrome_jni_headers/chrome/jni/PrefServiceBridge_jni.h"
#define JNI_PrefServiceBridge_GetAboutVersionStrings \
  JNI_PrefServiceBridge_GetAboutVersionStrings_ChromiumImpl
#include "../../../../../chrome/browser/android/preferences/pref_service_bridge.cc"
#undef JNI_PrefServiceBridge_GetAboutVersionStrings

static ScopedJavaLocalRef<jobject> JNI_PrefServiceBridge_GetAboutVersionStrings(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  JNI_PrefServiceBridge_GetAboutVersionStrings_ChromiumImpl(env, obj);

  std::string os_version = version_info::GetOSType();
  os_version += " " + AndroidAboutAppInfo::GetOsInfo();

  base::android::BuildInfo* android_build_info =
      base::android::BuildInfo::GetInstance();
  std::string application(android_build_info->host_package_label());
  application.append(" ");
  application.append(android_build_info->package_version_name());
  application.append(", Chromium ");
  application.append(version_info::GetVersionNumber());

  return Java_PrefServiceBridge_createAboutVersionStrings(
      env, ConvertUTF8ToJavaString(env, application),
      ConvertUTF8ToJavaString(env, os_version));
}
