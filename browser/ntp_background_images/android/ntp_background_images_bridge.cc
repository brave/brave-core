/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background_images/android/ntp_background_images_bridge.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ntp_background_images/view_counter_service_factory.h"
#include "brave/build/android/jni_headers/NTPBackgroundImagesBridge_jni.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;
using ntp_background_images::ViewCounterServiceFactory;

namespace ntp_background_images {

NTPBackgroundImagesBridgeFactory::NTPBackgroundImagesBridgeFactory()
    : BrowserContextKeyedServiceFactory(
          "NTPBackgroundImagesBridge",
          BrowserContextDependencyManager::GetInstance()) {}
NTPBackgroundImagesBridgeFactory::~NTPBackgroundImagesBridgeFactory() {}

// static
NTPBackgroundImagesBridge*
NTPBackgroundImagesBridgeFactory::GetForProfile(Profile* profile) {
  return static_cast<NTPBackgroundImagesBridge*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
NTPBackgroundImagesBridgeFactory*
NTPBackgroundImagesBridgeFactory::GetInstance() {
  return base::Singleton<NTPBackgroundImagesBridgeFactory>::get();
}

KeyedService* NTPBackgroundImagesBridgeFactory::BuildServiceInstanceFor(
      content::BrowserContext* context) const {
  return new NTPBackgroundImagesBridge(Profile::FromBrowserContext(context));
}

bool
NTPBackgroundImagesBridgeFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace ntp_background_images

NTPBackgroundImagesBridge::NTPBackgroundImagesBridge(Profile* profile)
    : profile_(profile),
      view_counter_service_(ViewCounterServiceFactory::GetForProfile(profile_)),
      background_images_service_(
          g_brave_browser_process->ntp_background_images_service()) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  java_object_.Reset(Java_NTPBackgroundImagesBridge_create(
      AttachCurrentThread(), reinterpret_cast<intptr_t>(this)));

  background_images_service_->AddObserver(this);
}

NTPBackgroundImagesBridge::~NTPBackgroundImagesBridge() {
  background_images_service_->RemoveObserver(this);
  Java_NTPBackgroundImagesBridge_destroy(AttachCurrentThread(), java_object_);
}

static base::android::ScopedJavaLocalRef<jobject>
JNI_NTPBackgroundImagesBridge_GetInstance(JNIEnv* env,
                                      const JavaParamRef<jobject>& j_profile) {
  auto* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  return ntp_background_images::NTPBackgroundImagesBridgeFactory::GetInstance()
             ->GetForProfile(profile)->GetJavaObject();
}

base::android::ScopedJavaLocalRef<jobject>
NTPBackgroundImagesBridge::GetJavaObject() {
  return base::android::ScopedJavaLocalRef<jobject>(java_object_);
}

void NTPBackgroundImagesBridge::RegisterPageView(
    JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (view_counter_service_)
    view_counter_service_->RegisterPageView();
}

base::android::ScopedJavaLocalRef<jobject>
NTPBackgroundImagesBridge::CreateWallpaper() {
  JNIEnv* env = AttachCurrentThread();

  auto data = view_counter_service_
      ? view_counter_service_->GetCurrentWallpaperForDisplay()
      : base::Value();
  if (data.is_none())
    return base::android::ScopedJavaLocalRef<jobject>();

  // TODO(bridiver) - need to either expose these constants or change this
  // to a struct instead of base::Value
  auto* image_path = data.FindStringPath("wallpaperImagePath");
  auto* logo_image_path = data.FindStringPath("logo.imagePath");
  if (!image_path || !logo_image_path)
    return base::android::ScopedJavaLocalRef<jobject>();

  auto focal_point_x = data.FindIntPath("focalPoint.x");
  auto focal_point_y = data.FindIntPath("focalPoint.y");
  auto* logo_destination_url = data.FindStringPath("logo.destinationUrl");

  return Java_NTPBackgroundImagesBridge_createWallpaper(
      env,
      ConvertUTF8ToJavaString(env, *image_path),
      focal_point_x ? *focal_point_x : 0,
      focal_point_y ? *focal_point_y : 0,
      ConvertUTF8ToJavaString(env, *logo_image_path),
      ConvertUTF8ToJavaString(env, logo_destination_url ? *logo_destination_url
                                                        : ""));
}

base::android::ScopedJavaLocalRef<jobject>
NTPBackgroundImagesBridge::GetCurrentWallpaper(
    JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return CreateWallpaper();
}

void NTPBackgroundImagesBridge::OnUpdated(NTPBackgroundImagesData* data) {
  // Don't have interest about in-effective component data update.
  if (data == view_counter_service_->GetCurrentBrandedWallpaperData())
    return;

  JNIEnv* env = AttachCurrentThread();
  Java_NTPBackgroundImagesBridge_onUpdated(env, java_object_);
}
