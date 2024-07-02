/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/logging.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/build/android/jni_headers/WalletNativeUtils_jni.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "chrome/browser/profiles/profile.h"

namespace chrome {
namespace android {

static void JNI_WalletNativeUtils_ResetWallet(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  DCHECK(profile);

  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile);
  if (brave_wallet_service) {
    brave_wallet_service->Reset();
  }
}

static jboolean JNI_WalletNativeUtils_IsUnstoppableDomainsTld(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& domain) {
  auto domain_string = base::android::ConvertJavaStringToUTF8(env, domain);

  return decentralized_dns::IsUnstoppableDomainsTLD(domain_string);
}

static jboolean JNI_WalletNativeUtils_IsEnsTld(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& domain) {
  auto domain_string = base::android::ConvertJavaStringToUTF8(env, domain);

  return decentralized_dns::IsENSTLD(domain_string);
}

static jboolean JNI_WalletNativeUtils_IsSnsTld(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& domain) {
  auto domain_string = base::android::ConvertJavaStringToUTF8(env, domain);

  return decentralized_dns::IsSnsTLD(domain_string);
}

}  // namespace android
}  // namespace chrome
