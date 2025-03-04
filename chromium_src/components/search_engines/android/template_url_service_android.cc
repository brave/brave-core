/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/search_engines/android/template_url_service_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"

#define DoesDefaultSearchEngineHaveLogo \
  DoesDefaultSearchEngineHaveLogo_ChromiumImpl
#include "src/components/search_engines/android/template_url_service_android.cc"
#undef DoesDefaultSearchEngineHaveLogo

#include "brave/components/search_engines/android/jni_headers/BraveTemplateUrlService_jni.h"

jboolean TemplateUrlServiceAndroid::DoesDefaultSearchEngineHaveLogo(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (IsDefaultSearchEngineGoogle(env, obj))
    return false;

  return DoesDefaultSearchEngineHaveLogo_ChromiumImpl(env, obj);
}

jboolean TemplateUrlServiceAndroid::AddSearchEngine(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& search_engine_title,
    const base::android::JavaParamRef<jstring>& search_engine_keyword,
    const base::android::JavaParamRef<jstring>& search_engine_url) {
  LOG(ERROR) << "brave_search : "
             << "TemplateUrlServiceAndroid::AddSearchEngine";
  const TemplateURL* existing = template_url_service_->GetTemplateURLForKeyword(
      base::android::ConvertJavaStringToUTF16(env, search_engine_title));
  if (existing) {
    return false;
  }

  TemplateURLData template_url_data;
  template_url_data.SetShortName(
      base::android::ConvertJavaStringToUTF16(env, search_engine_title));
  template_url_data.SetKeyword(
      base::android::ConvertJavaStringToUTF16(env, search_engine_keyword));
  template_url_data.SetURL(
      base::android::ConvertJavaStringToUTF8(env, search_engine_url));
  TemplateURL* template_url = template_url_service_->Add(
      std::make_unique<TemplateURL>(template_url_data));
  return (template_url != nullptr);
}

void TemplateUrlServiceAndroid::RemoveSearchEngine(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& search_engine_keyword) {
  const TemplateURL* existing = template_url_service_->GetTemplateURLForKeyword(
      base::android::ConvertJavaStringToUTF16(env, search_engine_keyword));
  LOG(ERROR) << "brave_search : "
             << "TemplateUrlServiceAndroid::RemoveSearchEngine 1";
  if (existing) {
    LOG(ERROR) << "brave_search : "
               << "TemplateUrlServiceAndroid::RemoveSearchEngine 2";
    template_url_service_->Remove(existing);
  }
}
