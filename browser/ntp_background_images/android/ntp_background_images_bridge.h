/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_IMAGES_ANDROID_NTP_BACKGROUND_IMAGES_BRIDGE_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_IMAGES_ANDROID_NTP_BACKGROUND_IMAGES_BRIDGE_H_

#include <memory>
#include <string>

#include "base/android/jni_android.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/optional.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace ntp_background_images {
struct NTPBackgroundImagesData;
class ViewCounterService;
}

using ntp_background_images::NTPBackgroundImagesData;
using ntp_background_images::NTPBackgroundImagesService;
using ntp_background_images::ViewCounterService;

class NTPBackgroundImagesBridge : public NTPBackgroundImagesService::Observer,
                                 public KeyedService {
 public:
  explicit NTPBackgroundImagesBridge(Profile* profile);
  ~NTPBackgroundImagesBridge() override;

  void RegisterPageView(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj);
  base::android::ScopedJavaLocalRef<jobject> GetCurrentWallpaper(
      JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void GetTopSites(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj);

  base::android::ScopedJavaLocalRef<jobject> GetJavaObject();

 private:
  void OnUpdated(NTPBackgroundImagesData* data) override;
  base::android::ScopedJavaLocalRef<jobject> CreateWallpaper();

  Profile* profile_;
  ViewCounterService* view_counter_service_;
  NTPBackgroundImagesService* background_images_service_;
  base::android::ScopedJavaGlobalRef<jobject> java_object_;

  DISALLOW_COPY_AND_ASSIGN(NTPBackgroundImagesBridge);
};

namespace ntp_background_images {

class NTPBackgroundImagesBridgeFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static NTPBackgroundImagesBridgeFactory* GetInstance();
  static NTPBackgroundImagesBridge* GetForProfile(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<NTPBackgroundImagesBridgeFactory>;

  NTPBackgroundImagesBridgeFactory();
  ~NTPBackgroundImagesBridgeFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;

  DISALLOW_COPY_AND_ASSIGN(NTPBackgroundImagesBridgeFactory);
};

}  // namespace ntp_background_images

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_IMAGES_ANDROID_NTP_BACKGROUND_IMAGES_BRIDGE_H_
