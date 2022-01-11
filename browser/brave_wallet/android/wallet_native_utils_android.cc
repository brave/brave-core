/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/logging.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/build/android/jni_headers/WalletNativeUtils_jni.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"

namespace chrome {
namespace android {

static void JNI_WalletNativeUtils_ResetWallet(JNIEnv* env) {
  auto* profile = ProfileManager::GetActiveUserProfile();
  DCHECK(profile);

  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile);
  if (brave_wallet_service)
    brave_wallet_service->Reset();
}

}  // namespace android
}  // namespace chrome
