/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_ANDROID_BRAVE_WALLET_NATIVE_WORKER_H_
#define BRAVE_BROWSER_BRAVE_WALLET_ANDROID_BRAVE_WALLET_NATIVE_WORKER_H_

#include <jni.h>

#include "base/android/jni_weak_ref.h"
#include "base/memory/weak_ptr.h"

namespace chrome {
namespace android {
class BraveWalletNativeWorker {
 public:
  BraveWalletNativeWorker(JNIEnv* env,
                          const base::android::JavaRef<jobject>& obj);
  ~BraveWalletNativeWorker();

  void Destroy(JNIEnv* env,
               const base::android::JavaParamRef<jobject>& jcaller);
  base::android::ScopedJavaLocalRef<jstring> CreateWallet(
      JNIEnv* env, const base::android::JavaParamRef<jstring>& password);
  void LockWallet(JNIEnv* env);
  bool UnlockWallet(JNIEnv* env,
                    const base::android::JavaParamRef<jstring>& password);

 private:
  JavaObjectWeakGlobalRef weak_java_brave_wallet_native_worker_;
  base::WeakPtrFactory<BraveWalletNativeWorker> weak_factory_;
};
}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_BRAVE_WALLET_ANDROID_BRAVE_WALLET_NATIVE_WORKER_H_
