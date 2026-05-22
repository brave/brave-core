/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/android/features_jni_headers/BraveAccountFeatures_jni.h"
#include "brave/components/brave_account/features.h"
#include "third_party/jni_zero/jni_zero.h"

namespace brave_account {

static jboolean JNI_BraveAccountFeatures_IsBraveAccountEnabled(JNIEnv* env) {
  return features::IsBraveAccountEnabled();
}

}  // namespace brave_account

DEFINE_JNI(BraveAccountFeatures)
