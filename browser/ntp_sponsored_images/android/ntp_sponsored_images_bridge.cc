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
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ntp_sponsored_images/view_counter_service_factory.h"
#include "brave/components/ntp_sponsored_images/browser/view_counter_service.h"
#include "brave/build/android/jni_headers/NTPSponsoredImagesBridge_jni.h"
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
using ntp_sponsored_images::NTPSponsoredImagesData;
using ntp_sponsored_images::NTPSponsoredImagesService;
using ntp_sponsored_images::ViewCounterService;
using ntp_sponsored_images::ViewCounterServiceFactory;

namespace ntp_sponsored_images {

NTPSponsoredImagesBridgeFactory::NTPSponsoredImagesBridgeFactory()
    : BrowserContextKeyedServiceFactory(
          "NTPSponsoredImagesBridge",
          BrowserContextDependencyManager::GetInstance()) {}
NTPSponsoredImagesBridgeFactory::~NTPSponsoredImagesBridgeFactory() {}

// static
NTPSponsoredImagesBridge*
NTPSponsoredImagesBridgeFactory::GetForProfile(Profile* profile) {
  return static_cast<NTPSponsoredImagesBridge*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
NTPSponsoredImagesBridgeFactory*
NTPSponsoredImagesBridgeFactory::GetInstance() {
  return base::Singleton<NTPSponsoredImagesBridgeFactory>::get();
}

KeyedService* NTPSponsoredImagesBridgeFactory::BuildServiceInstanceFor(
      content::BrowserContext* context) const {
  return new NTPSponsoredImagesBridge(Profile::FromBrowserContext(context));
}

bool
NTPSponsoredImagesBridgeFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace

NTPSponsoredImagesBridge::NTPSponsoredImagesBridge(Profile* profile)
    : profile_(profile),
      view_counter_service_(ViewCounterServiceFactory::GetForProfile(profile_)),
      sponsored_images_service_(
          g_brave_browser_process->ntp_sponsored_images_service()) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  java_object_.Reset(Java_NTPSponsoredImagesBridge_create(
      AttachCurrentThread(), reinterpret_cast<intptr_t>(this)));

  sponsored_images_service_->AddObserver(this);
}

NTPSponsoredImagesBridge::~NTPSponsoredImagesBridge() {
  sponsored_images_service_->RemoveObserver(this);
  Java_NTPSponsoredImagesBridge_destroy(AttachCurrentThread(), java_object_);
}

static base::android::ScopedJavaLocalRef<jobject>
JNI_NTPSponsoredImagesBridge_GetInstance(JNIEnv* env,
                                      const JavaParamRef<jobject>& j_profile) {
  auto* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  return ntp_sponsored_images::NTPSponsoredImagesBridgeFactory::GetInstance()
             ->GetForProfile(profile)->GetJavaObject();
}

base::android::ScopedJavaLocalRef<jobject>
NTPSponsoredImagesBridge::GetJavaObject() {
  return base::android::ScopedJavaLocalRef<jobject>(java_object_);
}

void NTPSponsoredImagesBridge::RegisterPageView(
    JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (view_counter_service_)
    view_counter_service_->RegisterPageView();
}

base::android::ScopedJavaLocalRef<jobject>
NTPSponsoredImagesBridge::CreateWallpaper() {
  JNIEnv* env = AttachCurrentThread();

  auto data = view_counter_service_->GetCurrentWallpaperForDisplay();
  if (!view_counter_service_ || data.is_none())
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

  return Java_NTPSponsoredImagesBridge_createWallpaper(
      env,
      ConvertUTF8ToJavaString(env, *image_path),
      focal_point_x ? *focal_point_x : 0,
      focal_point_y ? *focal_point_y : 0,
      ConvertUTF8ToJavaString(env, *logo_image_path),
      ConvertUTF8ToJavaString(env, logo_destination_url ? *logo_destination_url
                                                        : ""));
}

base::android::ScopedJavaLocalRef<jobject>
NTPSponsoredImagesBridge::GetCurrentWallpaper(
    JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return CreateWallpaper();
}

void NTPSponsoredImagesBridge::OnUpdated(NTPSponsoredImagesData* data) {
  JNIEnv* env = AttachCurrentThread();
  Java_NTPSponsoredImagesBridge_onUpdated(env, java_object_);
}
