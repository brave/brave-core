/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/version.h"
#include "brave/build/android/jni_headers/BlockchainRegistryFactory_jni.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace chrome {
namespace android {
static jlong JNI_BlockchainRegistryFactory_GetInterfaceToBlockchainRegistry(
    JNIEnv* env) {
  auto pending = brave_wallet::BlockchainRegistry::GetInstance()->MakeRemote();

  return static_cast<jlong>(pending.PassPipe().release().value());
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_BlockchainRegistryFactory_GetTokensIconsLocation(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);

  std::optional<base::Version> version =
      brave_wallet::GetLastInstalledWalletVersion();
  if (version) {
    base::FilePath path = profile->GetPath().DirName();
    path = path.AppendASCII(brave_wallet::kWalletBaseDirectory);
    path = path.AppendASCII(version->GetString());
    path = path.AppendASCII("images");

    return base::android::ConvertUTF8ToJavaString(env, path.MaybeAsASCII());
  }

  return base::android::ConvertUTF8ToJavaString(env, "");
}

}  // namespace android
}  // namespace chrome
