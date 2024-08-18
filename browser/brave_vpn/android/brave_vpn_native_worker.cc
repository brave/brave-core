/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/android/brave_vpn_native_worker.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/build/android/jni_headers/BraveVpnNativeWorker_jni.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"

using brave_vpn::BraveVpnService;

namespace {

BraveVpnService* GetBraveVpnService() {
  return brave_vpn::BraveVpnServiceFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile());
}

}  // namespace

namespace chrome {
namespace android {

BraveVpnNativeWorker::BraveVpnNativeWorker(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj)
    : weak_java_brave_vpn_native_worker_(env, obj), weak_factory_(this) {
  Java_BraveVpnNativeWorker_setNativePtr(env, obj,
                                         reinterpret_cast<intptr_t>(this));
}

BraveVpnNativeWorker::~BraveVpnNativeWorker() {}

void BraveVpnNativeWorker::Destroy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  delete this;
}

void BraveVpnNativeWorker::GetTimezonesForRegions(JNIEnv* env) {
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    brave_vpn_service->GetTimezonesForRegions(
        base::BindOnce(&BraveVpnNativeWorker::OnGetTimezonesForRegions,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveVpnNativeWorker::OnGetTimezonesForRegions(
    const std::string& timezones_json,
    bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveVpnNativeWorker_onGetTimezonesForRegions(
      env, weak_java_brave_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, timezones_json), success);
}

void BraveVpnNativeWorker::GetHostnamesForRegion(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& region) {
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    brave_vpn_service->GetHostnamesForRegion(
        base::BindOnce(&BraveVpnNativeWorker::OnGetHostnamesForRegion,
                       weak_factory_.GetWeakPtr()),
        base::android::ConvertJavaStringToUTF8(env, region));
  }
}

void BraveVpnNativeWorker::OnGetHostnamesForRegion(
    const std::string& hostnames_json,
    bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveVpnNativeWorker_onGetHostnamesForRegion(
      env, weak_java_brave_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, hostnames_json), success);
}

void BraveVpnNativeWorker::GetWireguardProfileCredentials(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& subscriber_credential,
    const base::android::JavaParamRef<jstring>& public_key,
    const base::android::JavaParamRef<jstring>& hostname) {
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    brave_vpn_service->GetWireguardProfileCredentials(
        base::BindOnce(&BraveVpnNativeWorker::OnGetWireguardProfileCredentials,
                       weak_factory_.GetWeakPtr()),
        base::android::ConvertJavaStringToUTF8(env, subscriber_credential),
        base::android::ConvertJavaStringToUTF8(env, public_key),
        base::android::ConvertJavaStringToUTF8(env, hostname));
  }
}

void BraveVpnNativeWorker::OnGetWireguardProfileCredentials(
    const std::string& wireguard_profile_credentials_json,
    bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveVpnNativeWorker_onGetWireguardProfileCredentials(
      env, weak_java_brave_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(
          env, wireguard_profile_credentials_json),
      success);
}

void BraveVpnNativeWorker::VerifyCredentials(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& hostname,
    const base::android::JavaParamRef<jstring>& client_id,
    const base::android::JavaParamRef<jstring>& subscriber_credential,
    const base::android::JavaParamRef<jstring>& api_auth_token) {
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    brave_vpn_service->VerifyCredentials(
        base::BindOnce(&BraveVpnNativeWorker::OnVerifyCredentials,
                       weak_factory_.GetWeakPtr()),
        base::android::ConvertJavaStringToUTF8(env, hostname),
        base::android::ConvertJavaStringToUTF8(env, client_id),
        base::android::ConvertJavaStringToUTF8(env, subscriber_credential),
        base::android::ConvertJavaStringToUTF8(env, api_auth_token));
  }
}

void BraveVpnNativeWorker::OnVerifyCredentials(
    const std::string& verify_credentials_json,
    bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveVpnNativeWorker_onVerifyCredentials(
      env, weak_java_brave_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, verify_credentials_json),
      success);
}

void BraveVpnNativeWorker::InvalidateCredentials(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& hostname,
    const base::android::JavaParamRef<jstring>& client_id,
    const base::android::JavaParamRef<jstring>& subscriber_credential,
    const base::android::JavaParamRef<jstring>& api_auth_token) {
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    brave_vpn_service->InvalidateCredentials(
        base::BindOnce(&BraveVpnNativeWorker::OnInvalidateCredentials,
                       weak_factory_.GetWeakPtr()),
        base::android::ConvertJavaStringToUTF8(env, hostname),
        base::android::ConvertJavaStringToUTF8(env, client_id),
        base::android::ConvertJavaStringToUTF8(env, subscriber_credential),
        base::android::ConvertJavaStringToUTF8(env, api_auth_token));
  }
}

void BraveVpnNativeWorker::OnInvalidateCredentials(
    const std::string& invalidate_credentials_json,
    bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveVpnNativeWorker_onInvalidateCredentials(
      env, weak_java_brave_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, invalidate_credentials_json),
      success);
}

void BraveVpnNativeWorker::GetSubscriberCredential(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& product_type,
    const base::android::JavaParamRef<jstring>& product_id,
    const base::android::JavaParamRef<jstring>& validation_method,
    const base::android::JavaParamRef<jstring>& purchase_token,
    const base::android::JavaParamRef<jstring>& bundle_id) {
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    brave_vpn_service->GetSubscriberCredential(
        base::BindOnce(&BraveVpnNativeWorker::OnGetSubscriberCredential,
                       weak_factory_.GetWeakPtr()),
        base::android::ConvertJavaStringToUTF8(env, product_type),
        base::android::ConvertJavaStringToUTF8(env, product_id),
        base::android::ConvertJavaStringToUTF8(env, validation_method),
        base::android::ConvertJavaStringToUTF8(env, purchase_token),
        base::android::ConvertJavaStringToUTF8(env, bundle_id));
  }
}

void BraveVpnNativeWorker::GetSubscriberCredentialV12(JNIEnv* env) {
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    brave_vpn_service->GetSubscriberCredentialV12(
        base::BindOnce(&BraveVpnNativeWorker::OnGetSubscriberCredential,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveVpnNativeWorker::OnGetSubscriberCredential(
    const std::string& subscriber_credential,
    bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveVpnNativeWorker_onGetSubscriberCredential(
      env, weak_java_brave_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, subscriber_credential),
      success);
}

void BraveVpnNativeWorker::VerifyPurchaseToken(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& purchase_token,
    const base::android::JavaParamRef<jstring>& product_id,
    const base::android::JavaParamRef<jstring>& product_type,
    const base::android::JavaParamRef<jstring>& bundle_id) {
  LOG(ERROR) << "brave_vpn" << "verifyPurchaseToken";
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    LOG(ERROR) << "brave_vpn" << "verifyPurchaseToken 2";
    LOG(ERROR) << "brave_vpn"
               << "verifyPurchaseToken 2 : purchase_token : " << purchase_token;
    LOG(ERROR) << "brave_vpn"
               << "verifyPurchaseToken 2 : product_id : " << product_id;
    LOG(ERROR) << "brave_vpn"
               << "verifyPurchaseToken 2 : product_type : " << product_type;
    LOG(ERROR) << "brave_vpn"
               << "verifyPurchaseToken 2 : bundle_id : " << bundle_id;
    brave_vpn_service->VerifyPurchaseToken(
        base::BindOnce(
            &BraveVpnNativeWorker::OnVerifyPurchaseToken,
            weak_factory_.GetWeakPtr(),
            base::android::ConvertJavaStringToUTF8(env, purchase_token),
            base::android::ConvertJavaStringToUTF8(env, product_id)),
        base::android::ConvertJavaStringToUTF8(env, purchase_token),
        base::android::ConvertJavaStringToUTF8(env, product_id),
        base::android::ConvertJavaStringToUTF8(env, product_type),
        base::android::ConvertJavaStringToUTF8(env, bundle_id));
  }
}

void BraveVpnNativeWorker::OnVerifyPurchaseToken(
    const std::string& purchase_token,
    const std::string& product_id,
    const std::string& json_response,
    bool success) {
  LOG(ERROR) << "brave_vpn"
             << "OnVerifyPurchaseToken 2 : json_response" + json_response;
  LOG(ERROR) << "brave_vpn" << "OnVerifyPurchaseToken 2";
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveVpnNativeWorker_onVerifyPurchaseToken(
      env, weak_java_brave_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, json_response),
      base::android::ConvertUTF8ToJavaString(env, purchase_token),
      base::android::ConvertUTF8ToJavaString(env, product_id), success);
}

jboolean BraveVpnNativeWorker::IsPurchasedUser(JNIEnv* env) {
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    return brave_vpn_service->is_purchased_user();
  }

  return false;
}

void BraveVpnNativeWorker::ReloadPurchasedState(JNIEnv* env) {
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    brave_vpn_service->ReloadPurchasedState();
  }
}

void BraveVpnNativeWorker::ReportForegroundP3A(JNIEnv* env) {
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    // Reporting a new session to P3A functions.
    brave_vpn_service->RecordP3A(true);
  }
}

void BraveVpnNativeWorker::ReportBackgroundP3A(JNIEnv* env,
                                               jlong session_start_time_ms,
                                               jlong session_end_time_ms) {
  BraveVpnService* brave_vpn_service = GetBraveVpnService();
  if (brave_vpn_service) {
    brave_vpn_service->RecordAndroidBackgroundP3A(session_start_time_ms,
                                                  session_end_time_ms);
  }
}

static void JNI_BraveVpnNativeWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new BraveVpnNativeWorker(env, jcaller);
}

}  // namespace android
}  // namespace chrome
