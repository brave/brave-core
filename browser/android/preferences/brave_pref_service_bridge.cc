/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/build/android/jni_headers/BravePrefServiceBridge_jni.h"

#include "build/build_config.h"
#include "base/android/jni_string.h"
#include "brave/browser/android/preferences/brave_prefs.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_perf_predictor/browser/buildflags.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#endif

using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using brave_shields::ControlType;

namespace {

PrefService* GetPrefService() {
  return ProfileManager::GetActiveUserProfile()
      ->GetOriginalProfile()
      ->GetPrefs();
}

Profile* GetOriginalProfile() {
  return ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
}

const char* GetPrefNameExposedToJava(int pref_index) {
  DCHECK_GE(pref_index, kBravePrefOffset);
  DCHECK_LT(pref_index, BravePref::BRAVE_PREF_NUM_PREFS);
  return kBravePrefsExposedToJava[pref_index - kBravePrefOffset];
}

}  // namespace


namespace chrome {
namespace android {

void JNI_BravePrefServiceBridge_SetHTTPSEEnabled(
    JNIEnv* env,
    jboolean enabled) {
  brave_shields::SetHTTPSEverywhereEnabled(
      HostContentSettingsMapFactory::GetForProfile(
          GetOriginalProfile()),
      enabled,
      GURL(),
      g_browser_process->local_state());
}

void JNI_BravePrefServiceBridge_SetAdBlockEnabled(
    JNIEnv* env,
    jboolean enabled) {
  brave_shields::SetAdControlType(
      HostContentSettingsMapFactory::GetForProfile(
          GetOriginalProfile()),
      static_cast<bool>(enabled) ? ControlType::BLOCK : ControlType::ALLOW,
      GURL(),
      g_browser_process->local_state());
}

void JNI_BravePrefServiceBridge_SetFingerprintingProtectionEnabled(
    JNIEnv* env,
    jboolean enabled) {
  brave_shields::SetFingerprintingControlType(
      HostContentSettingsMapFactory::GetForProfile(
          GetOriginalProfile()),
      static_cast<bool>(enabled) ? ControlType::BLOCK : ControlType::ALLOW,
      GURL(),
      g_browser_process->local_state());
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

jlong JNI_BravePrefServiceBridge_GetTrackersBlockedCount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  return profile->GetPrefs()->GetUint64(kTrackersBlocked);
}

jlong JNI_BravePrefServiceBridge_GetAdsBlockedCount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  return profile->GetPrefs()->GetUint64(kAdsBlocked);
}

jlong JNI_BravePrefServiceBridge_GetDataSaved(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  return profile->GetPrefs()->GetUint64(
      brave_perf_predictor::prefs::kBandwidthSavedBytes);
#endif
  return 0;
}

void JNI_BravePrefServiceBridge_SetOldTrackersBlockedCount(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile,
    jlong count) {
  if (count <= 0) {
    return;
  }
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  profile->GetPrefs()->SetUint64(kTrackersBlocked,
    count + profile->GetPrefs()->GetUint64(kTrackersBlocked));
}

void JNI_BravePrefServiceBridge_SetOldAdsBlockedCount(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile,
    jlong count) {
  if (count <= 0) {
    return;
  }
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  profile->GetPrefs()->SetUint64(kAdsBlocked,
    count + profile->GetPrefs()->GetUint64(kAdsBlocked));
}

void JNI_BravePrefServiceBridge_SetOldHttpsUpgradesCount(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile,
    jlong count) {
  if (count <= 0) {
    return;
  }
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  profile->GetPrefs()->SetUint64(kHttpsUpgrades,
    count + profile->GetPrefs()->GetUint64(kHttpsUpgrades));
}

void JNI_BravePrefServiceBridge_SetSafetynetCheckFailed(
    JNIEnv* env,
    jboolean value) {
  GetOriginalProfile()->GetPrefs()->SetBoolean(kSafetynetCheckFailed, value);
}

jboolean JNI_BravePrefServiceBridge_GetSafetynetCheckFailed(JNIEnv* env) {
  return GetOriginalProfile()->GetPrefs()->GetBoolean(kSafetynetCheckFailed);
}

void JNI_BravePrefServiceBridge_SetSafetynetStatus(
    JNIEnv* env,
    const JavaParamRef<jstring>& status) {
  g_browser_process->local_state()->SetString(
      kSafetynetStatus, ConvertJavaStringToUTF8(env, status));
}

void JNI_BravePrefServiceBridge_SetUseRewardsStagingServer(
    JNIEnv* env,
    jboolean enabled) {
  GetOriginalProfile()->GetPrefs()->SetBoolean(
      brave_rewards::prefs::kUseRewardsStagingServer, enabled);
}

jboolean JNI_BravePrefServiceBridge_GetUseRewardsStagingServer(JNIEnv* env) {
  return GetOriginalProfile()->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kUseRewardsStagingServer);
}

jboolean JNI_BravePrefServiceBridge_GetBooleanForContentSetting(JNIEnv* env,
    jint type) {
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(GetOriginalProfile());
  switch (content_settings->GetDefaultContentSetting((ContentSettingsType)type,
      nullptr)) {
    case CONTENT_SETTING_ALLOW:
      return true;
    case CONTENT_SETTING_BLOCK:
    default:
      return false;
  }

  return false;
}

void JNI_BravePrefServiceBridge_SetReferralAndroidFirstRunTimestamp(
    JNIEnv* env,
    jlong time) {
  return g_browser_process->local_state()->SetTime(
      kReferralAndroidFirstRunTimestamp, base::Time::FromJavaTime(time));
}

void JNI_BravePrefServiceBridge_SetReferralCheckedForPromoCodeFile(
    JNIEnv* env,
    jboolean value) {
  return g_browser_process->local_state()->SetBoolean(
      kReferralCheckedForPromoCodeFile, value);
}

void JNI_BravePrefServiceBridge_SetReferralInitialization(
    JNIEnv* env,
    jboolean value) {
  return g_browser_process->local_state()->SetBoolean(
      kReferralInitialization, value);
}

void JNI_BravePrefServiceBridge_SetReferralPromoCode(
    JNIEnv* env,
    const JavaParamRef<jstring>& promoCode) {
  return g_browser_process->local_state()->SetString(
      kReferralPromoCode, ConvertJavaStringToUTF8(env, promoCode));
}

void JNI_BravePrefServiceBridge_SetReferralDownloadId(
    JNIEnv* env,
    const JavaParamRef<jstring>& downloadId) {
  return g_browser_process->local_state()->SetString(
      kReferralDownloadID, ConvertJavaStringToUTF8(env, downloadId));
}

static jboolean JNI_BravePrefServiceBridge_GetBoolean(
    JNIEnv* env,
    const jint j_pref_index) {
  return GetPrefService()->GetBoolean(GetPrefNameExposedToJava(j_pref_index));
}

static void JNI_BravePrefServiceBridge_SetBoolean(JNIEnv* env,
                                             const jint j_pref_index,
                                             const jboolean j_value) {
  GetPrefService()->SetBoolean(GetPrefNameExposedToJava(j_pref_index), j_value);
}

static jint JNI_BravePrefServiceBridge_GetInteger(JNIEnv* env,
                                             const jint j_pref_index) {
  return GetPrefService()->GetInteger(GetPrefNameExposedToJava(j_pref_index));
}

static void JNI_BravePrefServiceBridge_SetInteger(JNIEnv* env,
                                             const jint j_pref_index,
                                             const jint j_value) {
  GetPrefService()->SetInteger(GetPrefNameExposedToJava(j_pref_index), j_value);
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_BravePrefServiceBridge_GetString(JNIEnv* env, const jint j_pref_index) {
  return base::android::ConvertUTF8ToJavaString(
      env, GetPrefService()->GetString(GetPrefNameExposedToJava(j_pref_index)));
}

static void JNI_BravePrefServiceBridge_SetString(
    JNIEnv* env,
    const jint j_pref_index,
    const base::android::JavaParamRef<jstring>& j_value) {
  GetPrefService()->SetString(
      GetPrefNameExposedToJava(j_pref_index),
      base::android::ConvertJavaStringToUTF8(env, j_value));
}

}  // namespace android
}  // namespace chrome
