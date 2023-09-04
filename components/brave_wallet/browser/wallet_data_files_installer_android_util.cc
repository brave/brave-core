/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/wallet_data_files_installer_android_util.h"

#include <jni.h>

#include "base/android/jni_android.h"
#include "brave/build/android/jni_headers/WalletDataFilesInstallerUtil_jni.h"

namespace brave_wallet {

bool IsBraveWalletConfiguredOnAndroid() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_WalletDataFilesInstallerUtil_isBraveWalletConfiguredOnAndroid(
      env);
}

}  // namespace brave_wallet
