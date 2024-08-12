/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_ANDROID_BRAVE_VPN_NATIVE_WORKER_H_
#define BRAVE_BROWSER_BRAVE_VPN_ANDROID_BRAVE_VPN_NATIVE_WORKER_H_

#include <jni.h>
#include <string>

#include "base/android/jni_weak_ref.h"
#include "base/memory/weak_ptr.h"

namespace chrome {
namespace android {
class BraveVpnNativeWorker {
 public:
  BraveVpnNativeWorker(JNIEnv* env, const base::android::JavaRef<jobject>& obj);
  ~BraveVpnNativeWorker();

  BraveVpnNativeWorker(const BraveVpnNativeWorker&) = delete;
  BraveVpnNativeWorker& operator=(const BraveVpnNativeWorker&) = delete;

  void Destroy(JNIEnv* env,
               const base::android::JavaParamRef<jobject>& jcaller);

  void GetTimezonesForRegions(JNIEnv* env);

  void OnGetTimezonesForRegions(const std::string& timezones_json,
                                bool success);

  void GetHostnamesForRegion(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& region);

  void OnGetHostnamesForRegion(const std::string& hostname_json, bool success);

  void GetWireguardProfileCredentials(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& subscriber_credential,
      const base::android::JavaParamRef<jstring>& public_key,
      const base::android::JavaParamRef<jstring>& hostname);

  void OnGetWireguardProfileCredentials(
      const std::string& wireguard_profile_credentials_json,
      bool success);

  void VerifyCredentials(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& hostname,
      const base::android::JavaParamRef<jstring>& client_id,
      const base::android::JavaParamRef<jstring>& subscriber_credential,
      const base::android::JavaParamRef<jstring>& api_auth_token);

  void OnVerifyCredentials(const std::string& verify_credentials_json,
                           bool success);
  void InvalidateCredentials(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& hostname,
      const base::android::JavaParamRef<jstring>& client_id,
      const base::android::JavaParamRef<jstring>& subscriber_credential,
      const base::android::JavaParamRef<jstring>& api_auth_token);

  void OnInvalidateCredentials(const std::string& invalidate_credentials_json,
                               bool success);

  void GetSubscriberCredential(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& product_type,
      const base::android::JavaParamRef<jstring>& product_id,
      const base::android::JavaParamRef<jstring>& validation_method,
      const base::android::JavaParamRef<jstring>& purchase_token,
      const base::android::JavaParamRef<jstring>& bundle_id);

  void GetSubscriberCredentialV12(JNIEnv* env);

  void OnGetSubscriberCredential(const std::string& subscriber_credential,
                                 bool success);

  void VerifyPurchaseToken(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& purchase_token,
      const base::android::JavaParamRef<jstring>& product_id,
      const base::android::JavaParamRef<jstring>& product_type,
      const base::android::JavaParamRef<jstring>& bundle_id);

  void OnVerifyPurchaseToken(const std::string& purchase_token,
                             const std::string& product_id,
                             const std::string& json_response,
                             bool success);

  void ReloadPurchasedState(JNIEnv* env);

  jboolean IsPurchasedUser(JNIEnv* env);

  void ReportForegroundP3A(JNIEnv* env);
  void ReportBackgroundP3A(JNIEnv* env,
                           jlong session_start_time_ms,
                           jlong session_end_time_ms);

 private:
  JavaObjectWeakGlobalRef weak_java_brave_vpn_native_worker_;
  base::WeakPtrFactory<BraveVpnNativeWorker> weak_factory_;
};
}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_BRAVE_VPN_ANDROID_BRAVE_VPN_NATIVE_WORKER_H_
