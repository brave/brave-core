/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"

#include "base/android/jni_android.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/android/chrome_jni_headers/BraveWalletProviderDelegateImplHelper_jni.h"
#include "content/public/browser/web_contents.h"

namespace brave_wallet {

void ShowPanel(content::WebContents*) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveWalletProviderDelegateImplHelper_showPanel(env);
}

void ShowWalletBackup() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveWalletProviderDelegateImplHelper_showWalletBackup(env);
}

void UnlockWallet() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveWalletProviderDelegateImplHelper_unlockWallet(env);
}

void ShowAccountCreation(content::WebContents*,
                         brave_wallet::mojom::CoinType coin_type) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveWalletProviderDelegateImplHelper_showAccountCreation(
      env, static_cast<int>(coin_type));
}

void WalletInteractionDetected(content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }
  Java_BraveWalletProviderDelegateImplHelper_walletInteractionDetected(
      base::android::AttachCurrentThread(), web_contents->GetJavaWebContents());
}

bool IsWeb3NotificationAllowed() {
  JNIEnv* env = base::android::AttachCurrentThread();

  return Java_BraveWalletProviderDelegateImplHelper_isWeb3NotificationAllowed(
      env);
}

}  // namespace brave_wallet

DEFINE_JNI(BraveWalletProviderDelegateImplHelper)
