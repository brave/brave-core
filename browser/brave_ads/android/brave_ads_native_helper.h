/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_ANDROID_BRAVE_ADS_NATIVE_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_ANDROID_BRAVE_ADS_NATIVE_HELPER_H_

#include "base/android/scoped_java_ref.h"

namespace brave_ads {

static jboolean JNI_BraveAdsNativeHelper_IsOptedInToNotificationAds(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android);

static void JNI_BraveAdsNativeHelper_SetOptedInToNotificationAds(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    jboolean opted_in);

static jboolean JNI_BraveAdsNativeHelper_IsSupportedRegion(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android);

static void JNI_BraveAdsNativeHelper_ClearData(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android);

static void JNI_BraveAdsNativeHelper_OnNotificationAdShown(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    const base::android::JavaParamRef<jstring>& j_notification_id);

static void JNI_BraveAdsNativeHelper_OnNotificationAdClosed(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    const base::android::JavaParamRef<jstring>& j_notification_id,
    jboolean j_by_user);

static void JNI_BraveAdsNativeHelper_OnNotificationAdClicked(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    const base::android::JavaParamRef<jstring>& j_notification_id);

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_ANDROID_BRAVE_ADS_NATIVE_HELPER_H_
