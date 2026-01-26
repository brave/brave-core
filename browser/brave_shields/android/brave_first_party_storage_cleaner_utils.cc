/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cstddef>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/browser/brave_shields/android/jni_headers/BraveFirstPartyStorageCleanerUtils_jni.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/profiles/profile.h"

namespace brave_shields {

static void JNI_BraveFirstPartyStorageCleanerUtils_CleanupTLDFirstPartyStorage(
    JNIEnv* env,
    const jni_zero::JavaRef<jobject>& tab_object) {
  CHECK(env);
  // Validate that GetNativeTab returned a valid TabAndroid pointer
  // GetNativeTab handles null JavaRef validation internally
  TabAndroid* tab_android = TabAndroid::GetNativeTab(env, tab_object);
  if (!tab_android) {
    return;
  }

  content::WebContents* web_contents = tab_android->web_contents();
  if (!web_contents) {
    return;
  }

  brave_shields::BraveShieldsTabHelper* brave_shields_tab_helper =
      brave_shields::BraveShieldsTabHelper::GetOrCreateForWebContents(
          web_contents);
  if (!brave_shields_tab_helper) {
    return;
  }

  brave_shields_tab_helper->EnforceSiteDataCleanup();
}

static void
JNI_BraveFirstPartyStorageCleanerUtils_TriggerCurrentAppStateNotification(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
  CHECK(env);
  Profile* profile = Profile::FromJavaObject(j_profile);
  if (!profile) {
    return;
  }

  auto* ephemeral_storage_service =
      EphemeralStorageServiceFactory::GetForContext(
          static_cast<content::BrowserContext*>(profile));
  if (!ephemeral_storage_service) {
    return;
  }

  ephemeral_storage_service->TriggerCurrentAppStateNotification();
}

bool IsAppInTaskStack() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_BraveFirstPartyStorageCleanerUtils_isAppInTaskStack(
      env, Java_BraveFirstPartyStorageCleanerUtils_getCurrentPackageName(env));
}

}  // namespace brave_shields
