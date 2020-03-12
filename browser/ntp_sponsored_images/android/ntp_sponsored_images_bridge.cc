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
#include "content/public/browser/browser_thread.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/android/java_bitmap.h"

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

namespace {

base::Optional<std::string> ReadFileToString(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents))
    return base::Optional<std::string>();
  return contents;
}

}  // namespace

NTPSponsoredImagesBridge::NTPImageRequest::NTPImageRequest(SkBitmap* bitmap)
    : bitmap_(bitmap) {}

NTPSponsoredImagesBridge::NTPImageRequest::~NTPImageRequest() {}

void NTPSponsoredImagesBridge::NTPImageRequest::OnImageDecoded(
    const SkBitmap& bitmap) {
  *bitmap_ = bitmap;
}

NTPSponsoredImagesBridge::NTPSponsoredImagesBridge(JNIEnv* env,
                               const JavaRef<jobject>& obj,
                               const JavaRef<jobject>& j_profile)
    : weak_java_ref_(env, obj),
      view_counter_service_(NULL),
      sponsored_images_service_(NULL),
      image_request_(new NTPImageRequest(&bitmap_)),
      logo_image_request_(new NTPImageRequest(&logo_bitmap_)),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  profile_ = ProfileAndroid::FromProfileAndroid(j_profile);
  view_counter_service_ = ViewCounterServiceFactory::GetForProfile(profile_);
  sponsored_images_service_ =
      g_brave_browser_process->ntp_sponsored_images_service();

  sponsored_images_service_->AddObserver(this);
  // preload the first image if available
  PreloadImageIfNeeded();
}

NTPSponsoredImagesBridge::~NTPSponsoredImagesBridge() {
  ImageDecoder::Cancel(image_request_.get());
  ImageDecoder::Cancel(logo_image_request_.get());
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
  // preload the next image
  PreloadImageIfNeeded();
}

void NTPSponsoredImagesBridge::PreloadImageIfNeeded() {
  auto data = view_counter_service_->GetCurrentWallpaper();
  if (data.is_none()) {
    return;
  }

  // TODO(bridiver) - need to either expose these constants or change this
  // to a struct instead of base::Value
  std::string* image_path = data.FindStringPath("wallpaperImagePath");
  if (!image_path)
    return;

  if (*image_path == image_path_)
    return;

  image_path_ = *image_path;

  ImageDecoder::Cancel(image_request_.get());
  bitmap_.reset();

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadFileToString, base::FilePath(image_path_)),
      base::BindOnce(&NTPSponsoredImagesBridge::OnGotImageFile,
                     weak_factory_.GetWeakPtr(),
                     base::Unretained(image_request_.get())));

  auto* logo_image_path = data.FindStringPath("logo.imagePath");
  if (!logo_image_path)
    return;

  if (*logo_image_path != logo_image_path_)
    return;

  logo_image_path_ = *logo_image_path;

  ImageDecoder::Cancel(logo_image_request_.get());
  logo_bitmap_.reset();

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadFileToString, base::FilePath(*logo_image_path)),
      base::BindOnce(&NTPSponsoredImagesBridge::OnGotImageFile,
                     weak_factory_.GetWeakPtr(),
                     base::Unretained(logo_image_request_.get())));
}

void NTPSponsoredImagesBridge::OnGotImageFile(
    ImageDecoder::ImageRequest* request,
    base::Optional<std::string> input) {
  if (!input)
    return;

  ImageDecoder::Start(request, *input);
}

base::android::ScopedJavaLocalRef<jobject>
NTPSponsoredImagesBridge::CreateWallpaper() {
  JNIEnv* env = AttachCurrentThread();

  auto data = view_counter_service_->GetCurrentWallpaperForDisplay();
  if (data.is_none() || bitmap_.isNull()) {
    return base::android::ScopedJavaLocalRef<jobject>();
  }

  // TODO(bridiver) - need to either expose these constants or change this
  // to a struct instead of base::Value
  auto focal_point_x = data.FindIntPath("focalPoint.x");
  auto focal_point_y = data.FindIntPath("focalPoint.y");

  return Java_NTPSponsoredImagesBridge_createWallpaper(
      env,
      gfx::ConvertToJavaBitmap(&bitmap_),
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
  // preload the first image
  PreloadImageIfNeeded();

  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jobject> obj = weak_java_ref_.get(env);
  if (obj.is_null())
    return;

  Java_NTPSponsoredImagesBridge_onUpdated(env, obj);
}
