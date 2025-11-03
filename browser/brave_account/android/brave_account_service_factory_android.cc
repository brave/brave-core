/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_account/brave_account_service_factory.h"

#include "base/android/jni_android.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "chrome/android/chrome_jni_headers/BraveAccountServiceFactory_jni.h"
#include "chrome/browser/profiles/profile.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace chrome::android {

static jlong JNI_BraveAccountServiceFactory_GetInterfaceToBraveAccountService(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  auto* brave_account_service =
      brave_account::BraveAccountServiceFactory::GetFor(profile);
  if (!brave_account_service) {
    return static_cast<jlong>(mojo::kInvalidHandleValue);
  }

  mojo::PendingRemote<brave_account::mojom::Authentication> pending_remote;
  brave_account_service->BindInterface(
      pending_remote.InitWithNewPipeAndPassReceiver());

  return static_cast<jlong>(pending_remote.PassPipe().release().value());
}

}  // namespace chrome::android
