/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"

#include "base/android/jni_android.h"
#include "brave/build/android/jni_headers/BraveWalletServiceFactory_jni.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace chrome {
namespace android {

namespace {
template <class T>
jlong BindWalletService(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  if (auto* brave_wallet_service =
          brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
              profile)) {
    mojo::PendingRemote<T> pending;
    brave_wallet_service->Bind(pending.InitWithNewPipeAndPassReceiver());
    return static_cast<jlong>(pending.PassPipe().release().value());
  }
  return static_cast<jlong>(mojo::kInvalidHandleValue);
}
}  // namespace

static jlong JNI_BraveWalletServiceFactory_GetInterfaceToBraveWalletService(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  return BindWalletService<brave_wallet::mojom::BraveWalletService>(
      env, profile_android);
}

static jlong JNI_BraveWalletServiceFactory_GetInterfaceToBraveWalletP3A(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  return BindWalletService<brave_wallet::mojom::BraveWalletP3A>(
      env, profile_android);
}

static jlong JNI_BraveWalletServiceFactory_GetInterfaceToJsonRpcService(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  return BindWalletService<brave_wallet::mojom::JsonRpcService>(
      env, profile_android);
}

static jlong JNI_BraveWalletServiceFactory_GetInterfaceToKeyringService(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  return BindWalletService<brave_wallet::mojom::KeyringService>(
      env, profile_android);
}

static jlong JNI_BraveWalletServiceFactory_GetInterfaceToTxService(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  return BindWalletService<brave_wallet::mojom::TxService>(env,
                                                           profile_android);
}

static jlong JNI_BraveWalletServiceFactory_GetInterfaceToEthTxManagerProxy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  return BindWalletService<brave_wallet::mojom::EthTxManagerProxy>(
      env, profile_android);
}

static jlong JNI_BraveWalletServiceFactory_GetInterfaceToSolanaTxManagerProxy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  return BindWalletService<brave_wallet::mojom::SolanaTxManagerProxy>(
      env, profile_android);
}

}  // namespace android
}  // namespace chrome
