/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/build/android/jni_headers/BraveWalletServiceFactory_jni.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "chrome/browser/profiles/profile.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace chrome {
namespace android {
static jlong JNI_BraveWalletServiceFactory_GetInterfaceToBraveWalletService(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  auto pending =
      brave_wallet::BraveWalletServiceFactory::GetInstance()->GetForContext(
          profile);

  return static_cast<jlong>(pending.PassPipe().release().value());
}

static jlong JNI_BraveWalletServiceFactory_GetInterfaceToBraveWalletP3A(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  auto pending = brave_wallet::BraveWalletServiceFactory::GetInstance()
                     ->GetServiceForContext(profile)
                     ->GetBraveWalletP3A()
                     ->MakeRemote();

  return static_cast<jlong>(pending.PassPipe().release().value());
}

}  // namespace android
}  // namespace chrome
