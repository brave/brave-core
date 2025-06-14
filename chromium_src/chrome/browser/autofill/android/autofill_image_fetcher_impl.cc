/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/autofill/android/autofill_image_fetcher_impl.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "chrome/browser/profiles/profile_key_android.h"
#include "ui/gfx/image/image.h"
#include "url/android/gurl_android.h"
#include "url/gurl.h"

// Must come after all headers that specialize FromJniType() / ToJniType().
#include "chrome/browser/autofill/android/jni_headers/AutofillImageFetcher_jni.h"

namespace autofill {

AutofillImageFetcherImpl::AutofillImageFetcherImpl(ProfileKey* key)
    : key_(key) {}
AutofillImageFetcherImpl::~AutofillImageFetcherImpl() = default;

void AutofillImageFetcherImpl::FetchCreditCardArtImagesForURLs(
    base::span<const GURL> image_urls,
    base::span<const AutofillImageFetcherBase::ImageSize> image_sizes) {
  JNIEnv* env = base::android::AttachCurrentThread();
  // Pass empty urls span. We have to make this call otherwise the compiler
  // complains about unused function.
  Java_AutofillImageFetcher_prefetchCardArtImages(
      env, GetOrCreateJavaImageFetcher(), {},
      base::android::ToJavaIntArray(env, {}));
}

void AutofillImageFetcherImpl::FetchPixAccountImagesForURLs(
    base::span<const GURL> image_urls) {
  // Pass empty urls span. We have to make this call otherwise the compiler
  // complains about unused function.
  Java_AutofillImageFetcher_prefetchPixAccountImages(
      base::android::AttachCurrentThread(), GetOrCreateJavaImageFetcher(), {});
}

void AutofillImageFetcherImpl::FetchValuableImagesForURLs(
    base::span<const GURL> image_urls) {}

const gfx::Image* AutofillImageFetcherImpl::GetCachedImageForUrl(
    const GURL& image_url,
    ImageType image_type) const {
  return nullptr;
}

base::android::ScopedJavaLocalRef<jobject>
AutofillImageFetcherImpl::GetOrCreateJavaImageFetcher() {
  // This fetcher is used on Java side without checking for null, so we can't
  // return an empty object ref.
  if (!java_image_fetcher_) {
    JNIEnv* env = base::android::AttachCurrentThread();
    java_image_fetcher_ = Java_AutofillImageFetcher_create(
        env, key_->GetProfileKeyAndroid()->GetJavaObject());
  }

  return base::android::ScopedJavaLocalRef<jobject>(java_image_fetcher_);
}

}  // namespace autofill
