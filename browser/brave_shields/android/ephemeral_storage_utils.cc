/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cstddef>

#include "base/android/jni_android.h"
#include "brave/browser/ephemeral_storage/android/jni_headers/BraveFirstPartyStorageCleanerUtils_jni.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "chrome/browser/android/tab_android.h"

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
      brave_shields::BraveShieldsTabHelper::GetOrCreateForWebContents(web_contents);
  if (!brave_shields_tab_helper) {
    return;
  }

  brave_shields_tab_helper->EnforceSiteDataCleanup();
}

}  // namespace brave_shields
