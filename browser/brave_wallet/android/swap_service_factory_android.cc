/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/swap_service_factory.h"

#include "base/android/jni_android.h"
#include "chrome/android/chrome_jni_headers/SwapServiceFactory_jni.h"
#include "chrome/browser/profiles/profile.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace chrome {
namespace android {
static jlong JNI_SwapServiceFactory_GetInterfaceToSwapService(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  auto pending =
      brave_wallet::SwapServiceFactory::GetInstance()->GetForContext(profile);

  return static_cast<jlong>(pending.PassPipe().release().value());
}

}  // namespace android
}  // namespace chrome
