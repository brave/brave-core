/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/notreached.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/build/android/jni_headers/BraveWalletProviderDelegateImplHelper_jni.h"
#include "content/public/browser/web_contents.h"

namespace brave_wallet {

void ShowPanel(content::WebContents*) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveWalletProviderDelegateImplHelper_showPanel(env);
}

void ShowWalletOnboarding(content::WebContents*) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveWalletProviderDelegateImplHelper_showWalletOnboarding(env);
}

void ShowAccountCreation(content::WebContents* web_contents,
                         const std::string& keyring_id) {
  NOTIMPLEMENTED();
}

void WalletInteractionDetected(content::WebContents* web_contents) {
  Java_BraveWalletProviderDelegateImplHelper_walletInteractionDetected(
      base::android::AttachCurrentThread(), web_contents->GetJavaWebContents());
}

bool IsWeb3NotificationAllowed() {
  JNIEnv* env = base::android::AttachCurrentThread();

  return Java_BraveWalletProviderDelegateImplHelper_isWeb3NotificationAllowed(
      env);
}

}  // namespace brave_wallet
