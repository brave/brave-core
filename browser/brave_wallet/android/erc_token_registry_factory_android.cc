/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "brave/build/android/jni_headers/ERCTokenRegistryFactory_jni.h"
#include "brave/components/brave_wallet/browser/erc_token_registry.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace chrome {
namespace android {
static jint JNI_ERCTokenRegistryFactory_GetInterfaceToERCTokenRegistry(
    JNIEnv* env) {
  auto pending = brave_wallet::ERCTokenRegistry::GetInstance()->MakeRemote();

  return static_cast<jint>(pending.PassPipe().release().value());
}

}  // namespace android
}  // namespace chrome
