/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/check_deref.h"
#include "brave/browser/brave_account/android/jni_headers/BraveAccountDialogMode_jni.h"
#include "brave/browser/brave_account/dialog_mode_holder.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "content/public/browser/web_contents.h"

namespace brave_account {

static void JNI_BraveAccountDialogMode_Set(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& java_web_contents,
    int32_t dialog_mode) {
  DialogModeHolder::SetDialogMode(
      CHECK_DEREF(content::WebContents::FromJavaWebContents(java_web_contents)),
      static_cast<mojom::DialogMode>(dialog_mode));
}

}  // namespace brave_account

DEFINE_JNI(BraveAccountDialogMode)
