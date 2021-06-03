/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/android/brave_wallet_native_worker.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/build/android/jni_headers/BraveWalletNativeWorker_jni.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"

namespace {

brave_wallet::BraveWalletService* GetBraveWalletService() {
  return brave_wallet::BraveWalletServiceFactory::GetForContext(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile());
}

}  // namespace

namespace chrome {
namespace android {

BraveWalletNativeWorker::BraveWalletNativeWorker(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj)
    : weak_java_brave_wallet_native_worker_(env, obj),
      weak_factory_(this) {
  Java_BraveWalletNativeWorker_setNativePtr(env,
                                            obj,
                                            reinterpret_cast<intptr_t>(this));
}

BraveWalletNativeWorker::~BraveWalletNativeWorker() {}

void BraveWalletNativeWorker::Destroy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  delete this;
}

base::android::ScopedJavaLocalRef<jstring>
    BraveWalletNativeWorker::CreateWallet(
        JNIEnv* env, const base::android::JavaParamRef<jstring>& password) {
  auto* keyring_controller =
      GetBraveWalletService()->keyring_controller();
  auto* keyring = keyring_controller->CreateDefaultKeyring(
      base::android::ConvertJavaStringToUTF8(env, password));
  if (keyring) {
    keyring->AddAccounts();
  }

  return base::android::ConvertUTF8ToJavaString(
      env, keyring_controller->GetMnemonicForDefaultKeyring());
}

void BraveWalletNativeWorker::LockWallet(JNIEnv* env) {
  auto* keyring_controller =
      GetBraveWalletService()->keyring_controller();
  keyring_controller->Lock();
}

bool BraveWalletNativeWorker::UnlockWallet(
    JNIEnv* env, const base::android::JavaParamRef<jstring>& password) {
  auto* keyring_controller =
      GetBraveWalletService()->keyring_controller();

  return keyring_controller->Unlock(
      base::android::ConvertJavaStringToUTF8(env, password));;
}

static void JNI_BraveWalletNativeWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new BraveWalletNativeWorker(env, jcaller);
}

}  // namespace android
}  // namespace chrome
