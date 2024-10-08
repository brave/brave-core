/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_ANDROID_NTP_BACKGROUND_IMAGES_BRIDGE_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_ANDROID_NTP_BACKGROUND_IMAGES_BRIDGE_H_

#include <memory>

#include "base/android/jni_android.h"
#include "base/compiler_specific.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace ntp_background_images {
struct NTPBackgroundImagesData;
struct NTPSponsoredImagesData;
class ViewCounterService;
}  // namespace ntp_background_images

using ntp_background_images::NTPBackgroundImagesData;
using ntp_background_images::NTPBackgroundImagesService;
using ntp_background_images::NTPSponsoredImagesData;
using ntp_background_images::ViewCounterService;

class NTPBackgroundImagesBridge : public NTPBackgroundImagesService::Observer,
                                  public KeyedService {
 public:
  explicit NTPBackgroundImagesBridge(Profile* profile);
  NTPBackgroundImagesBridge(const NTPBackgroundImagesBridge&) = delete;
  NTPBackgroundImagesBridge& operator=(const NTPBackgroundImagesBridge&) =
      delete;
  ~NTPBackgroundImagesBridge() override;

  void RegisterPageView(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj);
  void WallpaperLogoClicked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& jcreativeInstanceId,
      const base::android::JavaParamRef<jstring>& jdestinationUrl,
      const base::android::JavaParamRef<jstring>& jwallpaperId);
  base::android::ScopedJavaLocalRef<jobject> GetCurrentWallpaper(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void GetTopSites(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& obj);
  bool IsSuperReferral(JNIEnv* env,
                       const base::android::JavaParamRef<jobject>& obj);
  base::android::ScopedJavaLocalRef<jstring> GetSuperReferralThemeName(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  base::android::ScopedJavaLocalRef<jstring> GetSuperReferralCode(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  base::android::ScopedJavaLocalRef<jstring> GetReferralApiKey(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

  base::android::ScopedJavaLocalRef<jobject> GetJavaObject();

 private:
  void OnUpdated(NTPBackgroundImagesData* data) override;
  void OnUpdated(NTPSponsoredImagesData* data) override;
  void OnSuperReferralEnded() override;

  base::android::ScopedJavaLocalRef<jobject> CreateWallpaper(
      const base::Value::Dict& data);
  base::android::ScopedJavaLocalRef<jobject> CreateBrandedWallpaper(
      const base::Value::Dict& data);

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<ViewCounterService> view_counter_service_ = nullptr;
  raw_ptr<NTPBackgroundImagesService> background_images_service_ = nullptr;
  base::android::ScopedJavaGlobalRef<jobject> java_object_;
};

namespace ntp_background_images {

class NTPBackgroundImagesBridgeFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  NTPBackgroundImagesBridgeFactory(const NTPBackgroundImagesBridgeFactory&) =
      delete;
  NTPBackgroundImagesBridgeFactory& operator=(
      const NTPBackgroundImagesBridgeFactory&) = delete;

  static NTPBackgroundImagesBridgeFactory* GetInstance();
  static NTPBackgroundImagesBridge* GetForProfile(Profile* profile);

 private:
  friend base::NoDestructor<NTPBackgroundImagesBridgeFactory>;

  NTPBackgroundImagesBridgeFactory();
  ~NTPBackgroundImagesBridgeFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace ntp_background_images

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_ANDROID_NTP_BACKGROUND_IMAGES_BRIDGE_H_
