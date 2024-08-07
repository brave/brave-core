/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/android/brave_ads_native_helper.h"

#include <string>

#include "base/android/jni_string.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_ads/android/jni_headers/BraveAdsNativeHelper_jni.h"
#include "brave/components/brave_ads/core/public/ads_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {

// static
jboolean JNI_BraveAdsNativeHelper_IsOptedInToNotificationAds(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android) {
  Profile* profile = Profile::FromJavaObject(j_profile_android);
  return profile->GetPrefs()->GetBoolean(
      brave_ads::prefs::kOptedInToNotificationAds);
}

// static
void JNI_BraveAdsNativeHelper_SetOptedInToNotificationAds(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    jboolean should_enable_ads) {
  Profile* profile = Profile::FromJavaObject(j_profile_android);
  profile->GetPrefs()->SetBoolean(brave_ads::prefs::kOptedInToNotificationAds,
                                  should_enable_ads);
}

// static
jboolean JNI_BraveAdsNativeHelper_IsSupportedRegion(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android) {
  Profile* profile = Profile::FromJavaObject(j_profile_android);
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return false;
  }

  return brave_ads::IsSupportedRegion();
}

// static
void JNI_BraveAdsNativeHelper_ClearData(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android) {
  Profile* profile = Profile::FromJavaObject(j_profile_android);
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

  ads_service->ClearData();
}

// static
void JNI_BraveAdsNativeHelper_OnNotificationAdShown(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    const base::android::JavaParamRef<jstring>& j_notification_id) {
  Profile* profile = Profile::FromJavaObject(j_profile_android);
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

  const std::string notification_id =
      base::android::ConvertJavaStringToUTF8(env, j_notification_id);
  ads_service->OnNotificationAdShown(notification_id);
}

// static
void JNI_BraveAdsNativeHelper_OnNotificationAdClosed(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    const base::android::JavaParamRef<jstring>& j_notification_id,
    jboolean j_by_user) {
  Profile* profile = Profile::FromJavaObject(j_profile_android);
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

  const std::string notification_id =
      base::android::ConvertJavaStringToUTF8(env, j_notification_id);
  ads_service->OnNotificationAdClosed(notification_id, j_by_user);
}

// static
void JNI_BraveAdsNativeHelper_OnNotificationAdClicked(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    const base::android::JavaParamRef<jstring>& j_notification_id) {
  Profile* profile = Profile::FromJavaObject(j_profile_android);
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

  const std::string notification_id =
      base::android::ConvertJavaStringToUTF8(env, j_notification_id);
  ads_service->OnNotificationAdClicked(notification_id);
}

}  // namespace brave_ads
