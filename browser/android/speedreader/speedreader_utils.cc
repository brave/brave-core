/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/logging.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/build/android/jni_headers/BraveSpeedReaderUtils_jni.h"
#include "content/public/browser/web_contents.h"

namespace speedreader {

static void JNI_BraveSpeedReaderUtils_ToggleOnWebContent(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  auto* tab_helper = SpeedreaderTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    LOG(ERROR) << "speedreader_tab_helper: no tab_helper!";
    return;
  }
  tab_helper->MaybeToggleEnabledForSite(true);
}
}  // namespace speedreader
