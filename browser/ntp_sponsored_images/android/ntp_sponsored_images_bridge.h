/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BROWSER_ANDROID_NTP_SPONSORED_IMAGES_BRIDGE_H_
#define BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BROWSER_ANDROID_NTP_SPONSORED_IMAGES_BRIDGE_H_

#include <memory>
#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_service.h"
#include "chrome/browser/image_decoder.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "url/gurl.h"

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

 private:
  class NTPImageRequest : public ImageDecoder::ImageRequest {
   public:
    NTPImageRequest(SkBitmap* bitmap);
    ~NTPImageRequest() override;

    void OnImageDecoded(const SkBitmap& bitmap) override;

   private:
    SkBitmap* bitmap_;

    DISALLOW_COPY_AND_ASSIGN(NTPImageRequest);
  };

  void OnUpdated(NTPSponsoredImagesData* data) override;
  base::android::ScopedJavaLocalRef<jobject> CreateWallpaper();
  void PreloadImageIfNeeded();
  void OnGotImageFile(ImageDecoder::ImageRequest* request,
                      base::Optional<std::string> input);

  Profile* profile_;
  JavaObjectWeakGlobalRef weak_java_ref_;
  ViewCounterService* view_counter_service_;
  NTPSponsoredImagesService* sponsored_images_service_;

  std::unique_ptr<NTPImageRequest> image_request_;
  std::unique_ptr<NTPImageRequest> logo_image_request_;

  std::string image_path_;
  std::string logo_image_path_;
  SkBitmap bitmap_;
  SkBitmap logo_bitmap_;

  base::WeakPtrFactory<NTPSponsoredImagesBridge> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NTPSponsoredImagesBridge);
};

#endif  // BRAVE_BROWSER_NTP_SPONSORED_IMAGES_BROWSER_ANDROID_NTP_SPONSORED_IMAGES_BRIDGE_H_
