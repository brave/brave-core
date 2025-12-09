/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_origin/brave_origin_service_factory.h"

#include "base/android/jni_android.h"
#include "brave/browser/brave_origin/android/jni_headers/BraveOriginServiceFactory_jni.h"
#include "brave/components/brave_origin/brave_origin_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace brave {
namespace android {

static jlong
JNI_BraveOriginServiceFactory_GetInterfaceToBraveOriginSettingsHandler(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  mojo::PendingRemote<brave_origin::mojom::BraveOriginSettingsHandler> pending;
  if (profile) {
    auto* brave_origin_service =
        brave_origin::BraveOriginServiceFactory::GetForProfile(profile);
    if (brave_origin_service) {
      auto handler =
          std::make_unique<brave_origin::BraveOriginSettingsHandlerImpl>(
              brave_origin_service);
      mojo::PendingReceiver<brave_origin::mojom::BraveOriginSettingsHandler>
          receiver = pending.InitWithNewPipeAndPassReceiver();
      mojo::MakeSelfOwnedReceiver(std::move(handler), std::move(receiver));
    }
  }

  return static_cast<jlong>(pending.PassPipe().release().value());
}

}  // namespace android
}  // namespace brave
