/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_ANDROID_VPN_NATIVE_WORKER_H_
#define BRAVE_BROWSER_BRAVE_VPN_ANDROID_VPN_NATIVE_WORKER_H_

#include <jni.h>
#include <string>

#include "base/android/jni_weak_ref.h"
#include "base/memory/weak_ptr.h"

class VpnService;

namespace chrome {
namespace android {
class VpnNativeWorker {
 public:
  VpnNativeWorker(JNIEnv* env, const base::android::JavaRef<jobject>& obj);
  ~VpnNativeWorker();

  void Destroy(JNIEnv* env,
               const base::android::JavaParamRef<jobject>& jcaller);

  void GetAllServerRegions(JNIEnv* env,
                           const base::android::JavaParamRef<jobject>& jcaller);

  void OnGetAllServerRegions(const std::string& server_regions_json,
                             bool success);

  void GetTimezonesForRegions(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller);

  void OnGetTimezonesForRegions(const std::string& timezones_json,
                                bool success);

  void GetHostnamesForRegion(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller,
      const base::android::JavaParamRef<jstring>& region);

  void OnGetHostnamesForRegion(const std::string& hostname_json, bool success);

  void GetSubscriberCredential(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller,
      const base::android::JavaParamRef<jstring>& product_type,
      const base::android::JavaParamRef<jstring>& product_id,
      const base::android::JavaParamRef<jstring>& validation_method,
      const base::android::JavaParamRef<jstring>& purchase_token);

  void OnGetSubscriberCredential(const std::string& subscriber_credential,
                                 bool success);

  void VerifyPurchaseToken(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller,
      const base::android::JavaParamRef<jstring>& purchase_token,
      const base::android::JavaParamRef<jstring>& product_id,
      const base::android::JavaParamRef<jstring>& product_type);

  void OnVerifyPurchaseToken(const std::string& json_response, bool success);

 private:
  JavaObjectWeakGlobalRef weak_java_vpn_native_worker_;
  VpnService* vpn_service_;
  base::WeakPtrFactory<VpnNativeWorker> weak_factory_;
};
}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_BRAVE_VPN_ANDROID_VPN_NATIVE_WORKER_H_
