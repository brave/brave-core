/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/build/android/jni_headers/BravePrefServiceBridge_jni.h"

#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

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

}  // namespace android
}  // namespace chrome
