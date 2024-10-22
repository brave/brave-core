/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "brave/browser/webcompat_reporter/webcompat_reporter_service_factory.h"
#include "brave/build/android/jni_headers/WebcompatReporterServiceFactory_jni.h"
#include "chrome/browser/profiles/profile.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace chrome {

namespace android {

static jlong
JNI_WebcompatReporterServiceFactory_GetInterfaceToWebcompatReporterService(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  if (profile == nullptr) {
    return static_cast<jlong>(-1);
  }

  auto pending =
      webcompat_reporter::WebcompatReporterServiceFactory::GetInstance()
          ->GetHandlerForContext(profile);

  return static_cast<jlong>(pending.PassPipe().release().value());
}

}  // namespace android

}  // namespace chrome
