/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "brave/components/brave_account/features.h"
#include "chrome/android/chrome_jni_headers/BraveAccountFeature_jni.h"

namespace chrome::android {

static jboolean JNI_BraveAccountFeature_IsBraveAccountEnabled(JNIEnv* env) {
  return brave_account::features::IsBraveAccountEnabled();
}

}  // namespace chrome::android

DEFINE_JNI(BraveAccountFeature)
