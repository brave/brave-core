/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/build/android/jni_headers/BraveWalletProviderDelegateImplHelper_jni.h"

namespace content {
class WebContents;
}

namespace brave_wallet {

void ShowPanel(content::WebContents*) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveWalletProviderDelegateImplHelper_showPanel(env);
}

void ShowWalletOnboarding(content::WebContents*) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveWalletProviderDelegateImplHelper_showWalletOnboarding(env);
}

}  // namespace brave_wallet
