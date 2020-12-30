/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_ANDROID_BRAVE_ADS_NATIVE_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_ANDROID_BRAVE_ADS_NATIVE_HELPER_H_

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/browser/brave_ads/android/jni_headers/BraveAdsNativeHelper_jni.h"
#include "brave/components/brave_ads/browser/ads_notification_handler.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace brave_ads {

// static

static jboolean JNI_BraveAdsNativeHelper_IsBraveAdsEnabled(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android);

static jboolean JNI_BraveAdsNativeHelper_IsLocaleValid(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android);

static jboolean JNI_BraveAdsNativeHelper_IsSupportedLocale(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android);

static jboolean JNI_BraveAdsNativeHelper_IsNewlySupportedLocale(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android);

static void JNI_BraveAdsNativeHelper_SetAdsEnabled(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android);

static void JNI_BraveAdsNativeHelper_AdNotificationClicked(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    const base::android::JavaParamRef<jstring>& j_notification_id);

static void JNI_BraveAdsNativeHelper_AdNotificationDismissed(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    const base::android::JavaParamRef<jstring>& j_notification_id,
    jboolean j_by_user);

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_ANDROID_BRAVE_ADS_NATIVE_HELPER_H_
