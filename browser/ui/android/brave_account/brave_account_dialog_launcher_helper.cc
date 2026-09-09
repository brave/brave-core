/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/android/brave_account/brave_account_dialog_launcher_helper.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/build/android/jni_headers/BraveAccountDialogLauncherHelper_jni.h"
#include "content/public/browser/web_contents.h"

namespace brave_account {

void ShowBraveAccountDialog(content::WebContents* web_contents,
                            const std::string& url) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveAccountDialogLauncherHelper_showBraveAccountDialog(
      env, web_contents->GetJavaWebContents(),
      base::android::ConvertUTF8ToJavaString(env, url));
}

}  // namespace brave_account
