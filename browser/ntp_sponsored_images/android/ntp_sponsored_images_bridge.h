/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BROWSER_ANDROID_NTP_SPONSORED_IMAGES_BRIDGE_H_
#define BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BROWSER_ANDROID_NTP_SPONSORED_IMAGES_BRIDGE_H_

#include <memory>
#include <string>

#include "base/android/jni_android.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/optional.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_service.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace ntp_sponsored_images {
struct NTPSponsoredImagesData;
class ViewCounterService;
}

using ntp_sponsored_images::NTPSponsoredImagesData;
using ntp_sponsored_images::NTPSponsoredImagesService;
using ntp_sponsored_images::ViewCounterService;

class NTPSponsoredImagesBridge : public NTPSponsoredImagesService::Observer,
                                 public KeyedService {
 public:
  NTPSponsoredImagesBridge(Profile* profile);
  ~NTPSponsoredImagesBridge() override;

  static base::android::ScopedJavaLocalRef<jobject> GetInstance(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& j_profile);

  void RegisterPageView(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj);
  base::android::ScopedJavaLocalRef<jobject> GetCurrentWallpaper(
      JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

 private:
  void OnUpdated(NTPSponsoredImagesData* data) override;
  base::android::ScopedJavaLocalRef<jobject> CreateWallpaper();

  Profile* profile_;
  ViewCounterService* view_counter_service_;
  NTPSponsoredImagesService* sponsored_images_service_;
  base::android::ScopedJavaLocalRef<jobject> java_object_;

  DISALLOW_COPY_AND_ASSIGN(NTPSponsoredImagesBridge);
};

namespace ntp_sponsored_images {

class NTPSponsoredImagesBridgeFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static NTPSponsoredImagesBridgeFactory* GetInstance();
  static NTPSponsoredImagesBridge* GetForProfile(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<NTPSponsoredImagesBridgeFactory>;

  NTPSponsoredImagesBridgeFactory();
  ~NTPSponsoredImagesBridgeFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;

  DISALLOW_COPY_AND_ASSIGN(NTPSponsoredImagesBridgeFactory);
};

}  // namespace ntp_sponsored_images

#endif  // BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BROWSER_ANDROID_NTP_SPONSORED_IMAGES_BRIDGE_H_
