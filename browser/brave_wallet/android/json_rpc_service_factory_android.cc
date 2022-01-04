/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/build/android/jni_headers/JsonRpcServiceFactory_jni.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace chrome {
namespace android {
static jint JNI_JsonRpcServiceFactory_GetInterfaceToJsonRpcService(
    JNIEnv* env) {
  auto* profile = ProfileManager::GetActiveUserProfile();
  auto pending =
      brave_wallet::JsonRpcServiceFactory::GetInstance()->GetForContext(
          profile);

  return static_cast<jint>(pending.PassPipe().release().value());
}

}  // namespace android
}  // namespace chrome
