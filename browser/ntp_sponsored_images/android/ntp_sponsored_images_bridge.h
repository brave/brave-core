/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BROWSER_ANDROID_NTP_SPONSORED_IMAGES_BRIDGE_H_
#define BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BROWSER_ANDROID_NTP_SPONSORED_IMAGES_BRIDGE_H_

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_service.h"

class Profile;

namespace ntp_sponsored_images {
struct NTPSponsoredImagesData;
class ViewCounterService;
}

using ntp_sponsored_images::NTPSponsoredImagesData;
using ntp_sponsored_images::NTPSponsoredImagesService;
using ntp_sponsored_images::ViewCounterService;

class NTPSponsoredImagesBridge : public NTPSponsoredImagesService::Observer {
 public:
  NTPSponsoredImagesBridge(JNIEnv* env,
                 const base::android::JavaRef<jobject>& obj,
                 const base::android::JavaRef<jobject>& j_profile);
  ~NTPSponsoredImagesBridge() override;
  void Destroy(JNIEnv*, const base::android::JavaParamRef<jobject>&);


  void RegisterPageView(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj);
  base::android::ScopedJavaLocalRef<jobject> GetCurrentWallpaper(
      JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  // jboolean IsBrandedWallpaperActive(JNIEnv* env);
  // jboolean ShouldShowBrandedWallpaper(JNIEnv* env);

 private:
  base::android::ScopedJavaLocalRef<jobject> CreateWallpaper();
  void OnUpdated(NTPSponsoredImagesData* data) override;

  Profile* profile_;
  JavaObjectWeakGlobalRef weak_java_ref_;
  ViewCounterService* view_counter_service_;
  NTPSponsoredImagesService* sponsored_images_service_;

  DISALLOW_COPY_AND_ASSIGN(NTPSponsoredImagesBridge);
};

#endif  // BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BROWSER_ANDROID_NTP_SPONSORED_IMAGES_BRIDGE_H_
