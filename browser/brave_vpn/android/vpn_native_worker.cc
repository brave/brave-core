/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/android/vpn_native_worker.h"

#include <string>
#include <utility>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
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

std::string VpnNativeWorker::StdStrVecMapToJsonString(
    const std::map<std::string, std::vector<std::string>>& args) {
  std::string json_args;
  base::DictionaryValue dict;
  for (const auto& item : args) {
    auto inner_list = std::make_unique<base::ListValue>();
    if (!item.second.empty()) {
      for (const auto& it : item.second) {
        inner_list->Append(it);
      }
    }
    dict.SetList(item.first, std::move(inner_list));
  }
  base::JSONWriter::Write(dict, &json_args);
  return json_args;
}

std::string VpnNativeWorker::ConvertAssetsToJsonString(
    const std::map<std::string,
                   std::vector<std::map<std::string, std::string>>>& args) {
  std::string json_args;
  base::DictionaryValue dict;
  for (const auto& item : args) {
    auto inner_list = std::make_unique<base::ListValue>();
    if (!item.second.empty()) {
      for (const auto& it : item.second) {
        inner_list->Append(StdStrStrMapToJsonString(it));
      }
    }
    dict.SetList(item.first, std::move(inner_list));
  }
  base::JSONWriter::Write(dict, &json_args);
  return json_args;
}

std::string VpnNativeWorker::StdStrStrMapToJsonString(
    const std::map<std::string, std::string>& args) {
  std::string json_args;
  base::Value dict(base::Value::Type::DICTIONARY);
  for (const auto& item : args) {
    dict.SetStringKey(item.first, item.second);
  }
  base::JSONWriter::Write(dict, &json_args);
  return json_args;
}

void VpnNativeWorker::GetAllServerRegions(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  if (vpn_service_) {
    LOG(ERROR) << "NTP"
               << "VpnNativeWorker::GetAllServerRegions";
    vpn_service_->GetAllServerRegions(base::Bind(
        &VpnNativeWorker::OnGetAllServerRegions, weak_factory_.GetWeakPtr()));
  }
}

void VpnNativeWorker::OnGetAllServerRegions(
    const std::map<std::string, std::vector<std::string>>& balances,
    bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  std::string json_balances = StdStrVecMapToJsonString(balances);
  LOG(ERROR) << "NTP"
             << "VpnNativeWorker::OnGetAllServerRegions" << json_balances;
  Java_VpnNativeWorker_OnGetAllServerRegions(
      env, weak_java_vpn_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, json_balances), success);
}

static void JNI_VpnNativeWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new VpnNativeWorker(env, jcaller);
}

}  // namespace android
}  // namespace chrome
