/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/callback_android.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/notreached.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/build/android/jni_headers/BraveWalletProviderDelegateImplHelper_jni.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

using base::android::JavaParamRef;

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

void ShowWalletOnboarding(content::WebContents*) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveWalletProviderDelegateImplHelper_showWalletOnboarding(env);
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

static void JNI_BraveWalletProviderDelegateImplHelper_IsSolanaConnected(
    JNIEnv* env,
    const JavaParamRef<jobject>& jweb_contents,
    const base::android::JavaParamRef<jstring>& jaccount,
    const JavaParamRef<jobject>& jcallback) {
  content::RenderFrameHost* rfh = nullptr;
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  base::android::ScopedJavaGlobalRef<jobject> callback =
      base::android::ScopedJavaGlobalRef<jobject>(jcallback);
  const std::string account =
      base::android::ConvertJavaStringToUTF8(env, jaccount);
  if (!(rfh = web_contents->GetPrimaryMainFrame())) {
    base::android::RunBooleanCallbackAndroid(callback, false);
    return;
  }

  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    base::android::RunBooleanCallbackAndroid(callback, false);
    return;
  }

  base::android::RunBooleanCallbackAndroid(
      callback,
      tab_helper->IsSolanaAccountConnected(rfh->GetGlobalId(), account));
}

}  // namespace brave_wallet
