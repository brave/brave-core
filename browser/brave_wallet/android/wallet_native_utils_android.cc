/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/logging.h"
#include "brave/browser/brave_wallet/brave_wallet_reset.h"
#include "brave/build/android/jni_headers/WalletNativeUtils_jni.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"

namespace chrome {
namespace android {

static void JNI_WalletNativeUtils_ResetWallet(JNIEnv* env) {
  auto* profile = ProfileManager::GetActiveUserProfile();
  DCHECK(profile);

  brave_wallet::ResetWallet(profile);
}

}  // namespace android
}  // namespace chrome
