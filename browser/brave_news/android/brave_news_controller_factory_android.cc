/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_news/brave_news_controller_factory.h"

#include "base/android/jni_android.h"
#include "chrome/android/chrome_jni_headers/BraveNewsControllerFactory_jni.h"
#include "chrome/browser/profiles/profile.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace chrome {
namespace android {
static jlong JNI_BraveNewsControllerFactory_GetInterfaceToBraveNewsController(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  auto pending =
      brave_news::BraveNewsControllerFactory::GetInstance()->GetRemoteService(
          profile);
  if (!pending.is_valid()) {
    return 0;
  }
  auto passPipe = pending.PassPipe();

  return static_cast<jlong>(passPipe.release().value());
}

}  // namespace android
}  // namespace chrome
