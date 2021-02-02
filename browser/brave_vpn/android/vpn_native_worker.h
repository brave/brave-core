/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_ANDROID_VPN_NATIVE_WORKER_H_
#define BRAVE_BROWSER_BRAVE_VPN_ANDROID_VPN_NATIVE_WORKER_H_

#include <jni.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/android/jni_weak_ref.h"
#include "base/memory/weak_ptr.h"

// namespace vpn {
// class VpnService;
// }

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

  void OnGetAllServerRegions(
      const std::map<std::string, std::vector<std::string>>& regions,
      bool success);

 private:
  std::string StdStrStrMapToJsonString(
      const std::map<std::string, std::string>& args);
  std::string StdStrVecMapToJsonString(
      const std::map<std::string, std::vector<std::string>>& args);
  std::string ConvertAssetsToJsonString(
      const std::map<std::string,
                     std::vector<std::map<std::string, std::string>>>& args);

  JavaObjectWeakGlobalRef weak_java_vpn_native_worker_;
  VpnService* vpn_service_;
  base::WeakPtrFactory<VpnNativeWorker> weak_factory_;
};
}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_BRAVE_VPN_ANDROID_VPN_NATIVE_WORKER_H_
