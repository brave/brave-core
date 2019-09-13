/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/build/android/jni_headers/BravePrefServiceBridge_jni.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/profiles/profile_manager.h"

using base::android::JavaParamRef;
using brave_shields::ControlType;

namespace {

Profile* GetOriginalProfile() {
  return ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
}

}  // namespace

static void JNI_BravePrefServiceBridge_SetHTTPSEEnabled(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jboolean enabled) {
  brave_shields::SetHTTPSEverywhereEnabled(GetOriginalProfile(),
                                           enabled,
                                           GURL());
}

static void JNI_BravePrefServiceBridge_SetAdBlockEnabled(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jboolean enabled) {
  brave_shields::SetAdControlType(
      GetOriginalProfile(),
      static_cast<bool>(enabled) ? ControlType::BLOCK : ControlType::ALLOW,
      GURL());
}

static void JNI_BravePrefServiceBridge_SetFingerprintingProtectionEnabled(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jboolean enabled) {
  brave_shields::SetFingerprintingControlType(
      GetOriginalProfile(),
      static_cast<bool>(enabled) ? ControlType::BLOCK : ControlType::ALLOW,
      GURL());
}
