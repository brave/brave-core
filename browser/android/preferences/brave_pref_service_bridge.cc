/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/build/android/jni_headers/BravePrefServiceBridge_jni.h"

#include "base/android/jni_string.h"
#include "brave/components/brave_adaptive_captcha/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/decentralized_dns/core/pref_names.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "third_party/blink/public/common/peerconnection/webrtc_ip_handling_policy.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/pref_names.h"
#endif

using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using brave_shields::ControlType;

namespace {

Profile* GetOriginalProfile() {
  return ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
}

enum WebRTCIPHandlingPolicy {
  DEFAULT,
  DEFAULT_PUBLIC_AND_PRIVATE_INTERFACES,
  DEFAULT_PUBLIC_INTERFACE_ONLY,
  DISABLE_NON_PROXIED_UDP,
};

WebRTCIPHandlingPolicy GetWebRTCIPHandlingPolicy(
    const std::string& preference) {
  if (preference == blink::kWebRTCIPHandlingDefaultPublicAndPrivateInterfaces)
    return DEFAULT_PUBLIC_AND_PRIVATE_INTERFACES;
  if (preference == blink::kWebRTCIPHandlingDefaultPublicInterfaceOnly)
    return DEFAULT_PUBLIC_INTERFACE_ONLY;
  if (preference == blink::kWebRTCIPHandlingDisableNonProxiedUdp)
    return DISABLE_NON_PROXIED_UDP;
  return DEFAULT;
}

std::string GetWebRTCIPHandlingPreference(WebRTCIPHandlingPolicy policy) {
  if (policy == DEFAULT_PUBLIC_AND_PRIVATE_INTERFACES)
    return blink::kWebRTCIPHandlingDefaultPublicAndPrivateInterfaces;
  if (policy == DEFAULT_PUBLIC_INTERFACE_ONLY)
    return blink::kWebRTCIPHandlingDefaultPublicInterfaceOnly;
  if (policy == DISABLE_NON_PROXIED_UDP)
    return blink::kWebRTCIPHandlingDisableNonProxiedUdp;
  return blink::kWebRTCIPHandlingDefault;
}

}  // namespace

namespace chrome {
namespace android {

void JNI_BravePrefServiceBridge_SetCookiesBlockType(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& type) {
  brave_shields::SetCookieControlType(
      HostContentSettingsMapFactory::GetForProfile(GetOriginalProfile()),
      GetOriginalProfile()->GetPrefs(),
      brave_shields::ControlTypeFromString(
          base::android::ConvertJavaStringToUTF8(env, type)),
      GURL(), g_browser_process->local_state());
}

base::android::ScopedJavaLocalRef<jstring>
JNI_BravePrefServiceBridge_GetCookiesBlockType(JNIEnv* env) {
  brave_shields::ControlType control_type = brave_shields::GetCookieControlType(
      HostContentSettingsMapFactory::GetForProfile(GetOriginalProfile()),
      CookieSettingsFactory::GetForProfile(GetOriginalProfile()).get(), GURL());
  return base::android::ConvertUTF8ToJavaString(
      env, brave_shields::ControlTypeToString(control_type));
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
      kBackgroundVideoPlaybackEnabled, enabled);
}

jboolean JNI_BravePrefServiceBridge_GetBackgroundVideoPlaybackEnabled(
    JNIEnv* env) {
  return GetOriginalProfile()->GetPrefs()->GetBoolean(
      kBackgroundVideoPlaybackEnabled);
}

void JNI_BravePrefServiceBridge_SetDesktopModeEnabled(JNIEnv* env,
                                                      jboolean enabled) {
  return GetOriginalProfile()->GetPrefs()->SetBoolean(kDesktopModeEnabled,
                                                      enabled);
}

jboolean JNI_BravePrefServiceBridge_GetDesktopModeEnabled(JNIEnv* env) {
  return GetOriginalProfile()->GetPrefs()->GetBoolean(kDesktopModeEnabled);
}

jlong JNI_BravePrefServiceBridge_GetTrackersBlockedCount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
  Profile* profile = Profile::FromJavaObject(j_profile);
  return profile->GetPrefs()->GetUint64(kTrackersBlocked);
}

jlong JNI_BravePrefServiceBridge_GetAdsBlockedCount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
  Profile* profile = Profile::FromJavaObject(j_profile);
  return profile->GetPrefs()->GetUint64(kAdsBlocked);
}

jlong JNI_BravePrefServiceBridge_GetDataSaved(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
  Profile* profile = Profile::FromJavaObject(j_profile);
  return profile->GetPrefs()->GetUint64(
      brave_perf_predictor::prefs::kBandwidthSavedBytes);
}

void JNI_BravePrefServiceBridge_SetOldTrackersBlockedCount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile,
    jlong count) {
  if (count <= 0) {
    return;
  }
  Profile* profile = Profile::FromJavaObject(j_profile);
  profile->GetPrefs()->SetUint64(
      kTrackersBlocked,
      count + profile->GetPrefs()->GetUint64(kTrackersBlocked));
}

void JNI_BravePrefServiceBridge_SetOldAdsBlockedCount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile,
    jlong count) {
  if (count <= 0) {
    return;
  }
  Profile* profile = Profile::FromJavaObject(j_profile);
  profile->GetPrefs()->SetUint64(
      kAdsBlocked, count + profile->GetPrefs()->GetUint64(kAdsBlocked));
}

void JNI_BravePrefServiceBridge_SetOldHttpsUpgradesCount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile,
    jlong count) {
  if (count <= 0) {
    return;
  }
  Profile* profile = Profile::FromJavaObject(j_profile);
  profile->GetPrefs()->SetUint64(
      kHttpsUpgrades, count + profile->GetPrefs()->GetUint64(kHttpsUpgrades));
}

void JNI_BravePrefServiceBridge_SetSafetynetCheckFailed(JNIEnv* env,
                                                        jboolean value) {
  GetOriginalProfile()->GetPrefs()->SetBoolean(kSafetynetCheckFailed, value);
}

jboolean JNI_BravePrefServiceBridge_GetSafetynetCheckFailed(JNIEnv* env) {
  return GetOriginalProfile()->GetPrefs()->GetBoolean(kSafetynetCheckFailed);
}

void JNI_BravePrefServiceBridge_ResetPromotionLastFetchStamp(JNIEnv* env) {
  GetOriginalProfile()->GetPrefs()->SetUint64(
      brave_rewards::prefs::kPromotionLastFetchStamp, 0);
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
}

jint JNI_BravePrefServiceBridge_GetWebrtcPolicy(JNIEnv* env) {
  return static_cast<int>(
      GetWebRTCIPHandlingPolicy(GetOriginalProfile()->GetPrefs()->GetString(
          prefs::kWebRTCIPHandlingPolicy)));
}

void JNI_BravePrefServiceBridge_SetWebrtcPolicy(JNIEnv* env, jint policy) {
  GetOriginalProfile()->GetPrefs()->SetString(
      prefs::kWebRTCIPHandlingPolicy,
      GetWebRTCIPHandlingPreference((WebRTCIPHandlingPolicy)policy));
}

void JNI_BravePrefServiceBridge_SetNewsOptIn(JNIEnv* env, jboolean value) {
  GetOriginalProfile()->GetPrefs()->SetBoolean(
      brave_news::prefs::kBraveNewsOptedIn, value);
}

jboolean JNI_BravePrefServiceBridge_GetNewsOptIn(JNIEnv* env) {
  return GetOriginalProfile()->GetPrefs()->GetBoolean(
      brave_news::prefs::kBraveNewsOptedIn);
}

void JNI_BravePrefServiceBridge_SetShowNews(JNIEnv* env, jboolean value) {
  GetOriginalProfile()->GetPrefs()->SetBoolean(
      brave_news::prefs::kNewTabPageShowToday, value);
}

jboolean JNI_BravePrefServiceBridge_GetShowNews(JNIEnv* env) {
  return GetOriginalProfile()->GetPrefs()->GetBoolean(
      brave_news::prefs::kNewTabPageShowToday);
}

}  // namespace android
}  // namespace chrome
