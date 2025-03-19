/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_ANDROID_TEMPLATE_URL_SERVICE_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_ANDROID_TEMPLATE_URL_SERVICE_ANDROID_H_

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"

#define DoesDefaultSearchEngineHaveLogo                                   \
  DoesDefaultSearchEngineHaveLogo_ChromiumImpl(                           \
      JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);      \
  jboolean AddSearchEngine(                                               \
      JNIEnv* env,                                                        \
      const base::android::JavaParamRef<jstring>& search_engine_title,    \
      const base::android::JavaParamRef<jstring>& search_engine_keyword,  \
      const base::android::JavaParamRef<jstring>& search_engine_url);     \
  jboolean UpdateSearchEngine(                                            \
      JNIEnv* env,                                                        \
      const base::android::JavaParamRef<jstring>& existing_keyword,       \
      const base::android::JavaParamRef<jstring>& search_engine_title,    \
      const base::android::JavaParamRef<jstring>& search_engine_keyword,  \
      const base::android::JavaParamRef<jstring>& search_engine_url);     \
  void RemoveSearchEngine(                                                \
      JNIEnv* env,                                                        \
      const base::android::JavaParamRef<jstring>& search_engine_keyword); \
  jboolean DoesDefaultSearchEngineHaveLogo

#include "src/components/search_engines/android/template_url_service_android.h"  // IWYU pragma: export

#undef DoesDefaultSearchEngineHaveLogo

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_ANDROID_TEMPLATE_URL_SERVICE_ANDROID_H_
