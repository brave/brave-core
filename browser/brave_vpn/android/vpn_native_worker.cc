/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/android/vpn_native_worker.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/brave_vpn/vpn_service_factory.h"
#include "brave/build/android/jni_headers/VpnNativeWorker_jni.h"
#include "brave/components/brave_vpn/browser/vpn_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"

namespace chrome {
namespace android {

VpnNativeWorker::VpnNativeWorker(JNIEnv* env,
                                 const base::android::JavaRef<jobject>& obj)
    : weak_java_vpn_native_worker_(env, obj),
      vpn_service_(nullptr),
      weak_factory_(this) {
  Java_VpnNativeWorker_setNativePtr(env, obj, reinterpret_cast<intptr_t>(this));
  vpn_service_ = VpnServiceFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile());
}

VpnNativeWorker::~VpnNativeWorker() {}

void VpnNativeWorker::Destroy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  delete this;
}

void VpnNativeWorker::GetAllServerRegions(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  if (vpn_service_) {
    vpn_service_->GetAllServerRegions(base::Bind(
        &VpnNativeWorker::OnGetAllServerRegions, weak_factory_.GetWeakPtr()));
  }
}

void VpnNativeWorker::OnGetAllServerRegions(
    const std::string& server_regions_json,
    bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VpnNativeWorker_onGetAllServerRegions(
      env, weak_java_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, server_regions_json),
      success);
}

void VpnNativeWorker::GetTimezonesForRegions(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  if (vpn_service_) {
    vpn_service_->GetTimezonesForRegions(
        base::Bind(&VpnNativeWorker::OnGetTimezonesForRegions,
                   weak_factory_.GetWeakPtr()));
  }
}

void VpnNativeWorker::OnGetTimezonesForRegions(
    const std::string& timezones_json,
    bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VpnNativeWorker_onGetTimezonesForRegions(
      env, weak_java_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, timezones_json), success);
}

void VpnNativeWorker::GetHostnamesForRegion(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    const base::android::JavaParamRef<jstring>& region) {
  if (vpn_service_) {
    vpn_service_->GetHostnamesForRegion(
        base::Bind(&VpnNativeWorker::OnGetHostnamesForRegion,
                   weak_factory_.GetWeakPtr()),
        base::android::ConvertJavaStringToUTF8(env, region));
  }
}

void VpnNativeWorker::OnGetHostnamesForRegion(const std::string& hostnames_json,
                                              bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VpnNativeWorker_onGetHostnamesForRegion(
      env, weak_java_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, hostnames_json), success);
}

void VpnNativeWorker::GetSubscriberCredential(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    const base::android::JavaParamRef<jstring>& product_type,
    const base::android::JavaParamRef<jstring>& product_id,
    const base::android::JavaParamRef<jstring>& validation_method,
    const base::android::JavaParamRef<jstring>& purchase_token) {
  if (vpn_service_) {
    vpn_service_->GetSubscriberCredential(
        base::Bind(&VpnNativeWorker::OnGetSubscriberCredential,
                   weak_factory_.GetWeakPtr()),
        base::android::ConvertJavaStringToUTF8(env, product_type),
        base::android::ConvertJavaStringToUTF8(env, product_id),
        base::android::ConvertJavaStringToUTF8(env, validation_method),
        base::android::ConvertJavaStringToUTF8(env, purchase_token));
  }
}

void VpnNativeWorker::OnGetSubscriberCredential(
    const std::string& subscriber_credential,
    bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VpnNativeWorker_onGetSubscriberCredential(
      env, weak_java_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, subscriber_credential),
      success);
}

void VpnNativeWorker::VerifyPurchaseToken(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    const base::android::JavaParamRef<jstring>& purchase_token,
    const base::android::JavaParamRef<jstring>& product_id,
    const base::android::JavaParamRef<jstring>& product_type) {
  if (vpn_service_) {
    vpn_service_->VerifyPurchaseToken(
        base::Bind(&VpnNativeWorker::OnVerifyPurchaseToken,
                   weak_factory_.GetWeakPtr()),
        base::android::ConvertJavaStringToUTF8(env, purchase_token),
        base::android::ConvertJavaStringToUTF8(env, product_id),
        base::android::ConvertJavaStringToUTF8(env, product_type));
  }
}

void VpnNativeWorker::OnVerifyPurchaseToken(const std::string& json_response,
                                            bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VpnNativeWorker_onVerifyPurchaseToken(
      env, weak_java_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, json_response), success);
}

static void JNI_VpnNativeWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new VpnNativeWorker(env, jcaller);
}

}  // namespace android
}  // namespace chrome
