/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/search_engines/android/jni_headers/TemplateUrlServiceFactory_jni.h"

#define JNI_TemplateUrlServiceFactory_DoesDefaultSearchEngineHaveLogo \
  JNI_TemplateUrlServiceFactory_DoesDefaultSearchEngineHaveLogo_ChromiumImpl

#include "../../../../../chrome/browser/search_engines/template_url_service_factory_android.cc"

#undef JNI_TemplateUrlServiceFactory_DoesDefaultSearchEngineHaveLogo

static jboolean JNI_TemplateUrlServiceFactory_DoesDefaultSearchEngineHaveLogo(
    JNIEnv* env) {
  if (IsDefaultSearchEngineGoogle(env))
    return false;

  return
    JNI_TemplateUrlServiceFactory_DoesDefaultSearchEngineHaveLogo_ChromiumImpl(
      env);
}
