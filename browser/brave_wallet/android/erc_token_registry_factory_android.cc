/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/version.h"
#include "brave/build/android/jni_headers/ERCTokenRegistryFactory_jni.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/erc_token_registry.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace chrome {
namespace android {
static jint JNI_ERCTokenRegistryFactory_GetInterfaceToERCTokenRegistry(
    JNIEnv* env) {
  auto pending = brave_wallet::ERCTokenRegistry::GetInstance()->MakeRemote();

  return static_cast<jint>(pending.PassPipe().release().value());
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_ERCTokenRegistryFactory_GetTokensIconsLocation(JNIEnv* env) {
  auto* profile = ProfileManager::GetActiveUserProfile();

  absl::optional<base::Version> version =
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
