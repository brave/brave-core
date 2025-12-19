// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/tor/android/tor_service_bridge.h"

#include "base/android/jni_string.h"
#include "brave/browser/tor/jni_headers/TorServiceBridge_jni.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "url/gurl.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace tor {
namespace android {

TorServiceBridge::TorServiceBridge(JNIEnv* env,
                                   const JavaParamRef<jobject>& obj)
    : java_obj_(env, obj) {}

TorServiceBridge::~TorServiceBridge() = default;

static jlong JNI_TorServiceBridge_Init(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return reinterpret_cast<intptr_t>(new TorServiceBridge(env, obj));
}

void TorServiceBridge::Destroy(JNIEnv* env) {
  delete this;
}

void TorServiceBridge::UpdateProxyUri(
    JNIEnv* env,
    const JavaParamRef<jstring>& proxy_uri) {
  std::string uri = ConvertJavaStringToUTF8(env, proxy_uri);
  proxy_uri_ = uri;
  
  // Note: The actual proxy configuration is done through 
  // ProxyConfigServiceTor which is created per-profile.
  // This bridge stores the URI that should be used when
  // creating Tor profiles.
}

void TorServiceBridge::SetNewTorCircuit(
    JNIEnv* env,
    const JavaParamRef<jstring>& url) {
  std::string url_str = ConvertJavaStringToUTF8(env, url);
  GURL gurl(url_str);
  if (gurl.is_valid()) {
    net::ProxyConfigServiceTor::SetNewTorCircuit(gurl);
  }
}

jboolean TorServiceBridge::IsTorEnabled(JNIEnv* env) {
  return tor_enabled_;
}

void TorServiceBridge::SetTorEnabled(bool enabled) {
  tor_enabled_ = enabled;
}

std::string TorServiceBridge::GetProxyUri() const {
  return proxy_uri_;
}

// Static methods for global access
static TorServiceBridge* g_tor_service_bridge = nullptr;

// static
TorServiceBridge* TorServiceBridge::GetInstance() {
  return g_tor_service_bridge;
}

// static
void TorServiceBridge::SetInstance(TorServiceBridge* bridge) {
  g_tor_service_bridge = bridge;
}

}  // namespace android
}  // namespace tor
