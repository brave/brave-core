/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_account/brave_account_dialog_opener.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/browser/ui/android/brave_account/jni_headers/BraveAccountDialogOpener_jni.h"
#include "content/public/browser/web_contents.h"

namespace brave_account {

void OpenBraveAccountDialog(content::WebContents& web_contents,
                            const std::string& initiating_service_name,
                            mojom::DialogMode dialog_mode) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveAccountDialogOpener_openBraveAccountDialog(
      env, web_contents.GetJavaWebContents(),
      base::android::ConvertUTF8ToJavaString(env, initiating_service_name),
      std::to_underlying(dialog_mode));
}

}  // namespace brave_account
