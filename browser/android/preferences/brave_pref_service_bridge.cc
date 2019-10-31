/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/build/android/jni_headers/BravePrefServiceBridge_jni.h"

#include "base/android/jni_string.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using brave_shields::ControlType;

namespace {

Profile* GetOriginalProfile() {
  return ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
}

}  // namespace

namespace chrome {
namespace android {

void JNI_BravePrefServiceBridge_SetHTTPSEEnabled(
    JNIEnv* env,
    jboolean enabled) {
  brave_shields::SetHTTPSEverywhereEnabled(GetOriginalProfile(),
                                           enabled,
                                           GURL());
}

void JNI_BravePrefServiceBridge_SetAdBlockEnabled(
    JNIEnv* env,
    jboolean enabled) {
  brave_shields::SetAdControlType(
      GetOriginalProfile(),
      static_cast<bool>(enabled) ? ControlType::BLOCK : ControlType::ALLOW,
      GURL());
}

void JNI_BravePrefServiceBridge_SetFingerprintingProtectionEnabled(
    JNIEnv* env,
    jboolean enabled) {
  brave_shields::SetFingerprintingControlType(
      GetOriginalProfile(),
      static_cast<bool>(enabled) ? ControlType::BLOCK : ControlType::ALLOW,
      GURL());
}

void JNI_BravePrefServiceBridge_SetPlayYTVideoInBrowserEnabled(
    JNIEnv* env,
    jboolean enabled) {
  return GetOriginalProfile()->GetPrefs()->SetBoolean(
      kPlayYTVideoInBrowserEnabled, enabled);
}

jboolean JNI_BravePrefServiceBridge_GetPlayYTVideoInBrowserEnabled(
    JNIEnv* env) {
  return GetOriginalProfile()->GetPrefs()->GetBoolean(
      kPlayYTVideoInBrowserEnabled);
}

void JNI_BravePrefServiceBridge_SetBackgroundVideoPlaybackEnabled(
    JNIEnv* env,
    jboolean enabled) {
  return GetOriginalProfile()->GetPrefs()->SetBoolean(
      kBackgroundVideoPlaybackEnabled,
      enabled);
}

jboolean JNI_BravePrefServiceBridge_GetBackgroundVideoPlaybackEnabled(
    JNIEnv* env) {
  return GetOriginalProfile()->GetPrefs()->GetBoolean(
      kBackgroundVideoPlaybackEnabled);
}

void JNI_BravePrefServiceBridge_SetDesktopModeEnabled(
    JNIEnv* env,
    jboolean enabled) {
  return GetOriginalProfile()->GetPrefs()->SetBoolean(kDesktopModeEnabled,
                                                      enabled);
}

jboolean JNI_BravePrefServiceBridge_GetDesktopModeEnabled(
    JNIEnv* env) {
  return GetOriginalProfile()->GetPrefs()->GetBoolean(kDesktopModeEnabled);
}

int JNI_BravePrefServiceBridge_GetTrackersBlockedCount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  return profile->GetPrefs()->GetUint64(kTrackersBlocked);
}

int JNI_BravePrefServiceBridge_GetAdsBlockedCount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  return profile->GetPrefs()->GetUint64(kAdsBlocked);
}

int JNI_BravePrefServiceBridge_GetHttpsUpgradesCount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  return profile->GetPrefs()->GetUint64(kHttpsUpgrades);
}

ScopedJavaLocalRef<jstring> JNI_BravePrefServiceBridge_GetSyncDeviceId(
    JNIEnv* env) {
  return ConvertUTF8ToJavaString(env,
                                 GetOriginalProfile()->GetPrefs()->GetString(
                                     brave_sync::prefs::kSyncDeviceId));
}

void JNI_BravePrefServiceBridge_SetSyncDeviceName(
    JNIEnv* env,
    const JavaParamRef<jstring>& deviceName) {
  return GetOriginalProfile()->GetPrefs()->SetString(
      brave_sync::prefs::kSyncDeviceName,
      ConvertJavaStringToUTF8(env, deviceName));
}

ScopedJavaLocalRef<jstring> JNI_BravePrefServiceBridge_GetSyncDeviceName(
    JNIEnv* env) {
  return ConvertUTF8ToJavaString(env,
                                 GetOriginalProfile()->GetPrefs()->GetString(
                                     brave_sync::prefs::kSyncDeviceName));
}

void JNI_BravePrefServiceBridge_SetSyncSeed(
    JNIEnv* env,
    const JavaParamRef<jstring>& seed) {
  return GetOriginalProfile()->GetPrefs()->SetString(
      brave_sync::prefs::kSyncSeed, ConvertJavaStringToUTF8(env, seed));
}

ScopedJavaLocalRef<jstring> JNI_BravePrefServiceBridge_GetSyncSeed(
    JNIEnv* env) {
  return ConvertUTF8ToJavaString(env,
                                 GetOriginalProfile()->GetPrefs()->GetString(
                                     brave_sync::prefs::kSyncSeed));
}

void JNI_BravePrefServiceBridge_SetSafetynetCheckFailed(
    JNIEnv* env,
    jboolean value) {
  GetOriginalProfile()->GetPrefs()->SetBoolean(kSafetynetCheckFailed, value);
}

}  // namespace android
}  // namespace chrome
