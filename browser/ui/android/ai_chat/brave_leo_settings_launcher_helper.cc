// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/android/ai_chat/brave_leo_settings_launcher_helper.h"

#include "base/android/jni_android.h"
#include "brave/build/android/jni_headers/BraveLeoSettingsLauncherHelper_jni.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

void ShowBraveLeoSettings(content::WebContents* web_contents) {
  Java_BraveLeoSettingsLauncherHelper_showBraveLeoSettings(
      base::android::AttachCurrentThread(), web_contents->GetJavaWebContents());
}

void GoPremium(content::WebContents* web_contents) {
  Java_BraveLeoSettingsLauncherHelper_goPremium(
      base::android::AttachCurrentThread(), web_contents->GetJavaWebContents());
}

void ManagePremium(content::WebContents* web_contents) {
  Java_BraveLeoSettingsLauncherHelper_managePremium(
      base::android::AttachCurrentThread(), web_contents->GetJavaWebContents());
}

}  // namespace ai_chat
