/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/cosmetic_filters/cosmetic_filters_utils.h"

#include "brave/browser/cosmetic_filters/cosmetic_filters_tab_helper.h"
#include "brave/build/android/jni_headers/BraveCosmeticFiltersUtils_jni.h"
#include "chrome/browser/android/tab_android.h"

namespace cosmetic_filters {

static jboolean JNI_BraveCosmeticFiltersUtils_LaunchContentPickerForWebContent(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& tab) {
  TabAndroid* tab_android = TabAndroid::GetNativeTab(env, tab);
  content::WebContents* web_contents = tab_android->web_contents();
  if (!web_contents) {
    return false;
  }

  CosmeticFiltersTabHelper::LaunchContentPicker(web_contents);

  return true;
}

void ShowCustomFilterSettings() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveCosmeticFiltersUtils_showCustomFilterSettings(env);
}

int32_t GetThemeBackgroundColor() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_BraveCosmeticFiltersUtils_getThemeBackgroundColor(env);
}

}  // namespace cosmetic_filters
