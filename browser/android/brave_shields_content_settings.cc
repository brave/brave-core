/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_shields_content_settings.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/build/android/jni_headers/BraveShieldsContentSettings_jni.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

namespace chrome {
namespace android {


// That class is linked to a global toolbar. It's a one instance on Android
BraveShieldsContentSettings* g_brave_shields_content_settings = nullptr;

static void JNI_BraveShieldsContentSettings_Init(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  g_brave_shields_content_settings =
      new BraveShieldsContentSettings(env, jcaller);
}

BraveShieldsContentSettings::BraveShieldsContentSettings(JNIEnv* env,
    const base::android::JavaRef<jobject>& obj):
    weak_java_native_worker_(env, obj) {
  Java_BraveShieldsContentSettings_setNativePtr(env, obj,
      reinterpret_cast<intptr_t>(this));
}

BraveShieldsContentSettings::~BraveShieldsContentSettings() {
}

void BraveShieldsContentSettings::Destroy(JNIEnv* env) {
  g_brave_shields_content_settings = nullptr;
  delete this;
}

void BraveShieldsContentSettings::DispatchBlockedEventToJava(int tab_id,
        const std::string& block_type, const std::string& subresource) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveShieldsContentSettings_blockedEvent(env,
      weak_java_native_worker_.get(env), tab_id,
      base::android::ConvertUTF8ToJavaString(env, block_type),
      base::android::ConvertUTF8ToJavaString(env, subresource));
}

// static
void BraveShieldsContentSettings::DispatchBlockedEvent(int tab_id,
  const std::string& block_type, const std::string& subresource) {
  DCHECK(g_brave_shields_content_settings);
  if (!g_brave_shields_content_settings) {
    return;
  }
  g_brave_shields_content_settings->DispatchBlockedEventToJava(tab_id,
      block_type, subresource);
}

void JNI_BraveShieldsContentSettings_SetBraveShieldsEnabled(JNIEnv* env,
    jboolean enabled,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  brave_shields::SetBraveShieldsEnabled(
      ProfileAndroid::FromProfileAndroid(j_profile),
      enabled,
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));
}

jboolean JNI_BraveShieldsContentSettings_GetBraveShieldsEnabled(JNIEnv* env,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  return brave_shields::GetBraveShieldsEnabled(
      ProfileAndroid::FromProfileAndroid(j_profile),
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));
}

void JNI_BraveShieldsContentSettings_SetAdControlType(JNIEnv* env,
    const base::android::JavaParamRef<jstring>& type,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  brave_shields::SetAdControlType(
      ProfileAndroid::FromProfileAndroid(j_profile),
      brave_shields::ControlTypeFromString(
          base::android::ConvertJavaStringToUTF8(env, type)),
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));
}

base::android::ScopedJavaLocalRef<jstring>
    JNI_BraveShieldsContentSettings_GetAdControlType(JNIEnv* env,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  brave_shields::ControlType control_type =
      brave_shields::GetAdControlType(
          ProfileAndroid::FromProfileAndroid(j_profile),
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));

  return base::android::ConvertUTF8ToJavaString(env,
      brave_shields::ControlTypeToString(control_type));
}

void JNI_BraveShieldsContentSettings_SetCookieControlType(JNIEnv* env,
    const base::android::JavaParamRef<jstring>& type,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  brave_shields::SetCookieControlType(
      ProfileAndroid::FromProfileAndroid(j_profile),
      brave_shields::ControlTypeFromString(
          base::android::ConvertJavaStringToUTF8(env, type)),
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));
}

base::android::ScopedJavaLocalRef<jstring>
    JNI_BraveShieldsContentSettings_GetCookieControlType(JNIEnv* env,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  brave_shields::ControlType control_type =
      brave_shields::GetCookieControlType(
          ProfileAndroid::FromProfileAndroid(j_profile),
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));

  return base::android::ConvertUTF8ToJavaString(env,
      brave_shields::ControlTypeToString(control_type));
}

void JNI_BraveShieldsContentSettings_SetFingerprintingControlType(JNIEnv* env,
    const base::android::JavaParamRef<jstring>& type,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  brave_shields::SetFingerprintingControlType(
      ProfileAndroid::FromProfileAndroid(j_profile),
      brave_shields::ControlTypeFromString(
          base::android::ConvertJavaStringToUTF8(env, type)),
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));
}

base::android::ScopedJavaLocalRef<jstring>
    JNI_BraveShieldsContentSettings_GetFingerprintingControlType(JNIEnv* env,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  brave_shields::ControlType control_type =
      brave_shields::GetFingerprintingControlType(
          ProfileAndroid::FromProfileAndroid(j_profile),
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));

  return base::android::ConvertUTF8ToJavaString(env,
      brave_shields::ControlTypeToString(control_type));
}

void JNI_BraveShieldsContentSettings_SetHTTPSEverywhereEnabled(JNIEnv* env,
    jboolean enabled,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  brave_shields::SetHTTPSEverywhereEnabled(
      ProfileAndroid::FromProfileAndroid(j_profile),
      enabled,
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));
}

jboolean JNI_BraveShieldsContentSettings_GetHTTPSEverywhereEnabled(JNIEnv* env,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  return brave_shields::GetHTTPSEverywhereEnabled(
      ProfileAndroid::FromProfileAndroid(j_profile),
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));
}

void JNI_BraveShieldsContentSettings_SetNoScriptControlType(JNIEnv* env,
    const base::android::JavaParamRef<jstring>& type,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  brave_shields::SetNoScriptControlType(
      ProfileAndroid::FromProfileAndroid(j_profile),
      brave_shields::ControlTypeFromString(
          base::android::ConvertJavaStringToUTF8(env, type)),
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));
}

base::android::ScopedJavaLocalRef<jstring>
    JNI_BraveShieldsContentSettings_GetNoScriptControlType(JNIEnv* env,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jobject>& j_profile) {
  brave_shields::ControlType control_type =
      brave_shields::GetNoScriptControlType(
          ProfileAndroid::FromProfileAndroid(j_profile),
      GURL(base::android::ConvertJavaStringToUTF8(env, url)));

  return base::android::ConvertUTF8ToJavaString(env,
      brave_shields::ControlTypeToString(control_type));
}

}  // namespace android
}  // namespace chrome
