/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background_images/android/ntp_background_images_bridge.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ntp_background_images/view_counter_service_factory.h"
#include "brave/build/android/jni_headers/NTPBackgroundImagesBridge_jni.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#if BUILDFLAG(ENABLE_NTP_BACKGROUND_IMAGES)
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#endif
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
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

  if (background_images_service_)
    background_images_service_->AddObserver(this);
}

NTPBackgroundImagesBridge::~NTPBackgroundImagesBridge() {
  if (background_images_service_)
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

void NTPBackgroundImagesBridge::WallpaperLogoClicked(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& jcreativeInstanceId,
    const base::android::JavaParamRef<jstring>& jdestinationUrl,
    const base::android::JavaParamRef<jstring>& jwallpaperId) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (view_counter_service_) {
    view_counter_service_->BrandedWallpaperLogoClicked(
        base::android::ConvertJavaStringToUTF8(env, jcreativeInstanceId),
        base::android::ConvertJavaStringToUTF8(env, jdestinationUrl),
        base::android::ConvertJavaStringToUTF8(env, jwallpaperId));
  }
}

#if BUILDFLAG(ENABLE_NTP_BACKGROUND_IMAGES)
base::android::ScopedJavaLocalRef<jobject>
NTPBackgroundImagesBridge::CreateWallpaper(base::Value* data) {
  JNIEnv* env = AttachCurrentThread();

  auto* image_path =
      data->FindStringKey(ntp_background_images::kWallpaperImagePathKey);
  auto* author = data->FindStringKey(ntp_background_images::kImageAuthorKey);
  auto* link = data->FindStringKey(ntp_background_images::kImageLinkKey);

  return Java_NTPBackgroundImagesBridge_createWallpaper(
      env, ConvertUTF8ToJavaString(env, *image_path),
      ConvertUTF8ToJavaString(env, author ? *author : ""),
      ConvertUTF8ToJavaString(env, link ? *link : ""));
}
#endif

base::android::ScopedJavaLocalRef<jobject>
NTPBackgroundImagesBridge::CreateBrandedWallpaper(base::Value* data) {
  JNIEnv* env = AttachCurrentThread();

  const std::string wallpaper_id = base::GenerateGUID();
  view_counter_service_->BrandedWallpaperWillBeDisplayed(wallpaper_id);

  auto* image_path =
      data->FindStringKey(ntp_background_images::kWallpaperImagePathKey);
  auto* logo_image_path =
      data->FindStringPath(ntp_background_images::kLogoImagePath);
  if (!image_path || !logo_image_path)
    return base::android::ScopedJavaLocalRef<jobject>();

  auto focal_point_x =
      data->FindIntKey(ntp_background_images::kWallpaperFocalPointXKey)
          .value_or(0);
  auto focal_point_y =
      data->FindIntKey(ntp_background_images::kWallpaperFocalPointYKey)
          .value_or(0);
  auto* logo_destination_url =
      data->FindStringPath(ntp_background_images::kLogoDestinationURLPath);
  auto* theme_name = data->FindStringKey(ntp_background_images::kThemeNameKey);
  auto is_sponsored =
      data->FindBoolKey(ntp_background_images::kIsSponsoredKey).value_or(false);
  auto* creative_instance_id =
      data->FindStringKey(ntp_background_images::kCreativeInstanceIDKey);

  return Java_NTPBackgroundImagesBridge_createBrandedWallpaper(
      env, ConvertUTF8ToJavaString(env, *image_path), focal_point_x,
      focal_point_y, ConvertUTF8ToJavaString(env, *logo_image_path),
      ConvertUTF8ToJavaString(
          env, logo_destination_url ? *logo_destination_url : ""),
      ConvertUTF8ToJavaString(env, *theme_name), is_sponsored,
      ConvertUTF8ToJavaString(
          env, creative_instance_id ? *creative_instance_id : ""),
      ConvertUTF8ToJavaString(env, wallpaper_id));
}

void NTPBackgroundImagesBridge::GetTopSites(
  JNIEnv* env, const JavaParamRef<jobject>& obj) {
  std::vector<ntp_background_images::TopSite> top_sites = view_counter_service_
      ? view_counter_service_->GetTopSitesVectorData()
      : std::vector<ntp_background_images::TopSite>{};

  for (const auto& top_site : top_sites) {
    Java_NTPBackgroundImagesBridge_loadTopSitesData(
      env,
      ConvertUTF8ToJavaString(env, top_site.name),
      ConvertUTF8ToJavaString(env, top_site.destination_url),
      ConvertUTF8ToJavaString(env, top_site.background_color),
      ConvertUTF8ToJavaString(env, top_site.image_file.AsUTF8Unsafe()));
  }

  Java_NTPBackgroundImagesBridge_topSitesLoaded(env);
}

bool NTPBackgroundImagesBridge::IsSuperReferral(
  JNIEnv* env, const JavaParamRef<jobject>& obj) {
  if (view_counter_service_)
    return view_counter_service_->IsSuperReferral();
  return false;
}

base::android::ScopedJavaLocalRef<jstring>
NTPBackgroundImagesBridge::GetSuperReferralThemeName(
  JNIEnv* env, const JavaParamRef<jobject>& obj) {
  if (view_counter_service_)
    return ConvertUTF8ToJavaString(env,
      view_counter_service_->GetSuperReferralThemeName());
  return ConvertUTF8ToJavaString(env, "");
}

base::android::ScopedJavaLocalRef<jstring>
NTPBackgroundImagesBridge::GetSuperReferralCode(
  JNIEnv* env, const JavaParamRef<jobject>& obj) {
  if (view_counter_service_)
    return ConvertUTF8ToJavaString(env,
      view_counter_service_->GetSuperReferralCode());
  return ConvertUTF8ToJavaString(env, "");
}

base::android::ScopedJavaLocalRef<jstring>
NTPBackgroundImagesBridge::GetReferralApiKey(
  JNIEnv* env, const JavaParamRef<jobject>& obj) {
  return ConvertUTF8ToJavaString(env,
      brave_stats::GetAPIKey());
}

base::android::ScopedJavaLocalRef<jobject>
NTPBackgroundImagesBridge::GetCurrentWallpaper(
    JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto data = view_counter_service_
                  ? view_counter_service_->GetCurrentWallpaperForDisplay()
                  : base::Value();
  if (data.is_none())
    return base::android::ScopedJavaLocalRef<jobject>();

  auto is_background =
      data.FindBoolKey(ntp_background_images::kIsBackgroundKey).value();
  if (!is_background) {
    return CreateBrandedWallpaper(&data);
  } else {
#if BUILDFLAG(ENABLE_NTP_BACKGROUND_IMAGES)
    return CreateWallpaper(&data);
#else
    return base::android::ScopedJavaLocalRef<jobject>();
#endif
  }
}

#if BUILDFLAG(ENABLE_NTP_BACKGROUND_IMAGES)
void NTPBackgroundImagesBridge::OnUpdated(NTPBackgroundImagesData* data) {
  JNIEnv* env = AttachCurrentThread();
  Java_NTPBackgroundImagesBridge_onUpdated(env, java_object_);
}
#endif

void NTPBackgroundImagesBridge::OnUpdated(NTPSponsoredImagesData* data) {
  // Don't have interest about in-effective component data update.
  if (data != view_counter_service_->GetCurrentBrandedWallpaperData())
    return;

  JNIEnv* env = AttachCurrentThread();
  Java_NTPBackgroundImagesBridge_onUpdated(env, java_object_);
}

void NTPBackgroundImagesBridge::OnSuperReferralEnded() {
  // Android doesn't need to get this update.
}
