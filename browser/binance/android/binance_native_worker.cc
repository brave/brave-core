/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/binance/android/binance_native_worker.h"

#include <string>
#include <utility>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/binance/binance_service_factory.h"
#include "brave/build/android/jni_headers/BinanceNativeWorker_jni.h"
#include "brave/components/binance/browser/binance_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"

namespace chrome {
namespace android {

BinanceNativeWorker::BinanceNativeWorker(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj)
    : weak_java_binance_native_worker_(env, obj),
      binance_service_(nullptr),
      weak_factory_(this) {
  Java_BinanceNativeWorker_setNativePtr(env, obj,
                                        reinterpret_cast<intptr_t>(this));
  binance_service_ = BinanceServiceFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile());
}

BinanceNativeWorker::~BinanceNativeWorker() {}

void BinanceNativeWorker::Destroy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  delete this;
}

std::string BinanceNativeWorker::StdStrVecMapToJsonString(
    const std::map<std::string, std::vector<std::string>>& args) {
  std::string json_args;
  base::Value::Dict dict;
  for (const auto& item : args) {
    base::Value::List inner_list;
    if (!item.second.empty()) {
      for (const auto& it : item.second) {
        inner_list.Append(it);
      }
    }
    dict.Set(item.first, std::move(inner_list));
  }
  base::JSONWriter::Write(dict, &json_args);
  return json_args;
}

std::string BinanceNativeWorker::ConvertAssetsToJsonString(
    const std::map<std::string,
                   std::vector<std::map<std::string, std::string>>>& args) {
  std::string json_args;
  base::Value::Dict dict;
  for (const auto& item : args) {
    base::Value::List inner_list;
    if (!item.second.empty()) {
      for (const auto& it : item.second) {
        inner_list.Append(StdStrStrMapToJsonString(it));
      }
    }
    dict.Set(item.first, std::move(inner_list));
  }
  base::JSONWriter::Write(dict, &json_args);
  return json_args;
}

std::string BinanceNativeWorker::StdStrStrMapToJsonString(
    const std::map<std::string, std::string>& args) {
  std::string json_args;
  base::Value dict(base::Value::Type::DICTIONARY);
  for (const auto& item : args) {
    dict.SetStringKey(item.first, item.second);
  }
  base::JSONWriter::Write(dict, &json_args);
  return json_args;
}

base::android::ScopedJavaLocalRef<jstring>
BinanceNativeWorker::GetOAuthClientUrl(JNIEnv* env) {
  base::android::ScopedJavaLocalRef<jstring> url =
      base::android::ConvertUTF8ToJavaString(env, "");

  if (binance_service_) {
    url = base::android::ConvertUTF8ToJavaString(
        env, binance_service_->GetOAuthClientUrl());
  }

  return url;
}

void BinanceNativeWorker::GetAccessToken(JNIEnv* env) {
  if (binance_service_) {
    binance_service_->GetAccessToken(base::BindOnce(
        &BinanceNativeWorker::OnGetAccessToken, weak_factory_.GetWeakPtr()));
  }
}

bool BinanceNativeWorker::IsSupportedRegion(JNIEnv* env) {
  if (!binance_service_) {
    return false;
  }

  return binance_service_->IsSupportedRegion();
}

base::android::ScopedJavaLocalRef<jstring> BinanceNativeWorker::GetLocaleForURL(
    JNIEnv* env) {
  base::android::ScopedJavaLocalRef<jstring> locale =
      base::android::ConvertUTF8ToJavaString(env, "");

  if (binance_service_) {
    locale = base::android::ConvertUTF8ToJavaString(
        env, binance_service_->GetLocaleForURL());
  }

  return locale;
}

void BinanceNativeWorker::OnGetAccessToken(bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BinanceNativeWorker_OnGetAccessToken(
      env, weak_java_binance_native_worker_.get(env), success);
}

void BinanceNativeWorker::GetAccountBalances(JNIEnv* env) {
  if (binance_service_) {
    binance_service_->GetAccountBalances(
        base::BindOnce(&BinanceNativeWorker::OnGetAccountBalances,
                       weak_factory_.GetWeakPtr()));
  }
}

void BinanceNativeWorker::OnGetAccountBalances(
    const std::map<std::string, std::vector<std::string>>& balances,
    bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  std::string json_balances = StdStrVecMapToJsonString(balances);
  Java_BinanceNativeWorker_OnGetAccountBalances(
      env, weak_java_binance_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, json_balances), success);
}

void BinanceNativeWorker::GetConvertQuote(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& from,
    const base::android::JavaParamRef<jstring>& to,
    const base::android::JavaParamRef<jstring>& amount) {
  if (binance_service_) {
    binance_service_->GetConvertQuote(
        base::android::ConvertJavaStringToUTF8(env, from),
        base::android::ConvertJavaStringToUTF8(env, to),
        base::android::ConvertJavaStringToUTF8(env, amount),
        base::BindOnce(&BinanceNativeWorker::OnGetConvertQuote,
                       weak_factory_.GetWeakPtr()));
  }
}

void BinanceNativeWorker::OnGetConvertQuote(const std::string& quote_id,
                                            const std::string& quote_price,
                                            const std::string& total_fee,
                                            const std::string& total_amount) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BinanceNativeWorker_OnGetConvertQuote(
      env, weak_java_binance_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, quote_id),
      base::android::ConvertUTF8ToJavaString(env, quote_price),
      base::android::ConvertUTF8ToJavaString(env, total_fee),
      base::android::ConvertUTF8ToJavaString(env, total_amount));
}

void BinanceNativeWorker::GetCoinNetworks(JNIEnv* env) {
  if (binance_service_) {
    binance_service_->GetCoinNetworks(base::BindOnce(
        &BinanceNativeWorker::OnGetCoinNetworks, weak_factory_.GetWeakPtr()));
  }
}

void BinanceNativeWorker::OnGetCoinNetworks(
    const std::map<std::string, std::string>& networks) {
  JNIEnv* env = base::android::AttachCurrentThread();
  std::string json_networks = StdStrStrMapToJsonString(networks);
  Java_BinanceNativeWorker_OnGetCoinNetworks(
      env, weak_java_binance_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, json_networks));
}

void BinanceNativeWorker::GetDepositInfo(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& symbol,
    const base::android::JavaParamRef<jstring>& ticker_network) {
  if (binance_service_) {
    binance_service_->GetDepositInfo(
        base::android::ConvertJavaStringToUTF8(env, symbol),
        base::android::ConvertJavaStringToUTF8(env, ticker_network),
        base::BindOnce(&BinanceNativeWorker::OnGetDepositInfo,
                       weak_factory_.GetWeakPtr()));
  }
}

void BinanceNativeWorker::OnGetDepositInfo(const std::string& deposit_address,
                                           const std::string& deposit_tag,
                                           bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BinanceNativeWorker_OnGetDepositInfo(
      env, weak_java_binance_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, deposit_address),
      base::android::ConvertUTF8ToJavaString(env, deposit_tag), success);
}

void BinanceNativeWorker::ConfirmConvert(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& quote_id) {
  if (binance_service_) {
    binance_service_->ConfirmConvert(
        base::android::ConvertJavaStringToUTF8(env, quote_id),
        base::BindOnce(&BinanceNativeWorker::OnConfirmConvert,
                       weak_factory_.GetWeakPtr()));
  }
}

void BinanceNativeWorker::OnConfirmConvert(bool success,
                                           const std::string& message) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BinanceNativeWorker_OnConfirmConvert(
      env, weak_java_binance_native_worker_.get(env), success,
      base::android::ConvertUTF8ToJavaString(env, message));
}

void BinanceNativeWorker::GetConvertAssets(JNIEnv* env) {
  if (binance_service_) {
    binance_service_->GetConvertAssets(base::BindOnce(
        &BinanceNativeWorker::OnGetConvertAssets, weak_factory_.GetWeakPtr()));
  }
}

void BinanceNativeWorker::OnGetConvertAssets(
    const std::map<std::string,
                   std::vector<std::map<std::string, std::string>>>& assets) {
  JNIEnv* env = base::android::AttachCurrentThread();
  std::string json_assets = ConvertAssetsToJsonString(assets);
  Java_BinanceNativeWorker_OnGetConvertAssets(
      env, weak_java_binance_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, json_assets));
}

void BinanceNativeWorker::RevokeToken(JNIEnv* env) {
  if (binance_service_) {
    binance_service_->RevokeToken(base::BindOnce(
        &BinanceNativeWorker::OnRevokeToken, weak_factory_.GetWeakPtr()));
  }
}

void BinanceNativeWorker::OnRevokeToken(bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BinanceNativeWorker_OnRevokeToken(
      env, weak_java_binance_native_worker_.get(env), success);
}

void BinanceNativeWorker::SetAuthToken(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& auth_token) {
  if (binance_service_) {
    binance_service_->SetAuthToken(
        base::android::ConvertJavaStringToUTF8(env, auth_token));
  }
}

static void JNI_BinanceNativeWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new BinanceNativeWorker(env, jcaller);
}

}  // namespace android
}  // namespace chrome
