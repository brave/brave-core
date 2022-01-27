/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/search_engines/android/template_url_service_android.h"

#define DoesDefaultSearchEngineHaveLogo \
  DoesDefaultSearchEngineHaveLogo_ChromiumImpl

#include "src/components/search_engines/android/template_url_service_android.cc"

#undef DoesDefaultSearchEngineHaveLogo

jboolean TemplateUrlServiceAndroid::DoesDefaultSearchEngineHaveLogo(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (IsDefaultSearchEngineGoogle(env, obj))
    return false;

  return DoesDefaultSearchEngineHaveLogo_ChromiumImpl(env, obj);
}
