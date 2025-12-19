// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_TOR_ANDROID_TOR_SERVICE_BRIDGE_H_
#define BRAVE_BROWSER_TOR_ANDROID_TOR_SERVICE_BRIDGE_H_

#include <string>

#include "base/android/scoped_java_ref.h"

namespace tor {
namespace android {

// JNI bridge for Android Tor service.
// Provides interface between Java TorService and C++ ProxyConfigServiceTor.
class TorServiceBridge {
 public:
  TorServiceBridge(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& obj);
  ~TorServiceBridge();

  TorServiceBridge(const TorServiceBridge&) = delete;
  TorServiceBridge& operator=(const TorServiceBridge&) = delete;

  // Called when the Java bridge is being destroyed.
  void Destroy(JNIEnv* env);

  // Updates the SOCKS5 proxy URI for Tor.
  void UpdateProxyUri(JNIEnv* env,
                      const base::android::JavaParamRef<jstring>& proxy_uri);

  // Requests a new Tor circuit for the given URL.
  void SetNewTorCircuit(JNIEnv* env,
                        const base::android::JavaParamRef<jstring>& url);

  // Returns whether Tor is currently enabled.
  jboolean IsTorEnabled(JNIEnv* env);

  // Sets whether Tor is enabled.
  void SetTorEnabled(bool enabled);

  // Gets the current proxy URI.
  std::string GetProxyUri() const;

  // Static access for global bridge instance
  static TorServiceBridge* GetInstance();
  static void SetInstance(TorServiceBridge* bridge);

 private:
  base::android::ScopedJavaGlobalRef<jobject> java_obj_;
  std::string proxy_uri_;
  bool tor_enabled_ = false;
};

}  // namespace android
}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_ANDROID_TOR_SERVICE_BRIDGE_H_
