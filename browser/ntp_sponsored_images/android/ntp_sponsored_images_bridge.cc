/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_sponsored_images/android/ntp_sponsored_images_bridge.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ntp_sponsored_images/view_counter_service_factory.h"
#include "brave/components/ntp_sponsored_images/browser/view_counter_service.h"
#include "brave/build/android/jni_headers/NTPSponsoredImagesBridge_jni.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "content/public/browser/browser_thread.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;
using ntp_sponsored_images::NTPSponsoredImagesData;
using ntp_sponsored_images::NTPSponsoredImagesService;
using ntp_sponsored_images::ViewCounterService;
using ntp_sponsored_images::ViewCounterServiceFactory;

NTPSponsoredImagesBridge::NTPSponsoredImagesBridge(JNIEnv* env,
                               const JavaRef<jobject>& obj,
                               const JavaRef<jobject>& j_profile)
    : weak_java_ref_(env, obj),
      view_counter_service_(NULL),
      sponsored_images_service_(NULL) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  profile_ = ProfileAndroid::FromProfileAndroid(j_profile);
  view_counter_service_ = ViewCounterServiceFactory::GetForProfile(profile_);
  sponsored_images_service_ =
      g_brave_browser_process->ntp_sponsored_images_service();

  sponsored_images_service_->AddObserver(this);
}

NTPSponsoredImagesBridge::~NTPSponsoredImagesBridge() {
  sponsored_images_service_->RemoveObserver(this);
}

void NTPSponsoredImagesBridge::Destroy(JNIEnv*, const JavaParamRef<jobject>&) {
  delete this;
}

static jlong JNI_NTPSponsoredImagesBridge_Init(JNIEnv* env,
                                     const JavaParamRef<jobject>& obj,
                                     const JavaParamRef<jobject>& j_profile) {
  NTPSponsoredImagesBridge* delegate =
      new NTPSponsoredImagesBridge(env, obj, j_profile);
  return reinterpret_cast<intptr_t>(delegate);
}

void NTPSponsoredImagesBridge::RegisterPageView(
    JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  view_counter_service_->RegisterPageView();
}

base::android::ScopedJavaLocalRef<jobject>
NTPSponsoredImagesBridge::CreateWallpaper() {
  JNIEnv* env = AttachCurrentThread();

  auto data = view_counter_service_->GetCurrentWallpaper();
  if (data.is_none())
    return base::android::ScopedJavaLocalRef<jobject>();

  auto* image_url = data.FindStringPath("wallpaperImageUrl");
  auto focal_point_x = data.FindIntPath("focalPoint.x");
  auto focal_point_y = data.FindIntPath("focalPoint.y");

  if (!image_url)
    return base::android::ScopedJavaLocalRef<jobject>();

  ScopedJavaLocalRef<jstring> jurl = ConvertUTF8ToJavaString(env, *image_url);

  return Java_NTPSponsoredImagesBridge_createWallpaper(
      env,
      jurl,
      focal_point_x ? *focal_point_x : 0,
      focal_point_y ? *focal_point_y : 0);
}

base::android::ScopedJavaLocalRef<jobject>
NTPSponsoredImagesBridge::GetCurrentWallpaper(
    JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return CreateWallpaper();
}

void NTPSponsoredImagesBridge::OnUpdated(NTPSponsoredImagesData* data) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jobject> obj = weak_java_ref_.get(env);
  if (obj.is_null())
    return;

  Java_NTPSponsoredImagesBridge_onUpdated(env, obj, CreateWallpaper());
}
