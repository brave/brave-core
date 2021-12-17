/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_ANDROID_TEMPLATE_URL_SERVICE_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_ANDROID_TEMPLATE_URL_SERVICE_ANDROID_H_

#define DoesDefaultSearchEngineHaveLogo                              \
  DoesDefaultSearchEngineHaveLogo_ChromiumImpl(                      \
      JNIEnv* env, const base::android::JavaParamRef<jobject>& obj); \
  jboolean DoesDefaultSearchEngineHaveLogo

#include "src/components/search_engines/android/template_url_service_android.h"

#undef DoesDefaultSearchEngineHaveLogo

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_ANDROID_TEMPLATE_URL_SERVICE_ANDROID_H_
