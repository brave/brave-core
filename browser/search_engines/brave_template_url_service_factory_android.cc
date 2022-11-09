/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/android/java/org/chromium/chrome/browser/search_engines/jni_headers/BraveTemplateUrlServiceFactory_jni.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "components/search_engines/template_url_service.h"

static TemplateURLService* GetTemplateUrlService(Profile* profile) {
  return TemplateURLServiceFactory::GetForProfile(profile);
}

static Profile* GetProfileFromTabModelList() {
  for (const TabModel* model : TabModelList::models()) {
    if (model->IsActiveModel()) {
      return model->GetProfile();
    }
  }

  return ProfileManager::GetLastUsedProfileAllowedByPolicy();
}

static base::android::ScopedJavaLocalRef<jobject>
JNI_BraveTemplateUrlServiceFactory_GetTemplateUrlService(JNIEnv* env) {
  return GetTemplateUrlService(GetProfileFromTabModelList())->GetJavaObject();
}

static base::android::ScopedJavaLocalRef<jobject>
JNI_BraveTemplateUrlServiceFactory_GetTemplateUrlServiceByProfile(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  return GetTemplateUrlService(profile)->GetJavaObject();
}
