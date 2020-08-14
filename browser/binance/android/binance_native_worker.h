/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BINANCE_ANDROID_BINANCE_NATIVE_WORKER_H_
#define BRAVE_BROWSER_BINANCE_ANDROID_BINANCE_NATIVE_WORKER_H_

#include <jni.h>
#include <memory>
#include <map>
#include <string>
#include <vector>

#include "base/android/jni_weak_ref.h"
#include "base/memory/weak_ptr.h"

namespace binance {
class BinanceService;
}

namespace chrome {
namespace android {

class BinanceNativeWorker {
 public:
    BinanceNativeWorker(JNIEnv* env,
        const base::android::JavaRef<jobject>& obj);
    ~BinanceNativeWorker() override;

    void Destroy(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    base::android::ScopedJavaLocalRef<jstring> GetOAuthClientUrl(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    void GetAccessToken(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    void OnGetAccessToken(bool success);

    void GetAccountBalances(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    void OnGetAccountBalances(
        const std::map<std::string, std::vector<std::string>>& balances,
        bool success);

    void GetConvertQuote(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller,
        const base::android::JavaParamRef<jstring>& from,
        const base::android::JavaParamRef<jstring>& to,
        const base::android::JavaParamRef<jstring>& amount);

    void OnGetConvertQuote(
        const std::string& quote_id, const std::string& quote_price,
        const std::string& total_fee, const std::string& total_amount);

    void GetCoinNetworks(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    void OnGetCoinNetworks(
        const std::map<std::string, std::string>& networks);

    void GetDepositInfo(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller,
        const base::android::JavaParamRef<jstring>& symbol,
        const base::android::JavaParamRef<jstring>& ticker_network);

    void OnGetDepositInfo(
        const std::string& deposit_address,
        const std::string& deposit_tag,
        bool success);

    void ConfirmConvert(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller,
        const base::android::JavaParamRef<jstring>& quote_id);

    void OnConfirmConvert(
        bool success, const std::string& message);

    void GetConvertAssets(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    void OnGetConvertAssets(
        const std::map<std::string, std::vector<std::string>>& assets);

    void RevokeToken(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

    void OnRevokeToken(bool success);

 private:
    std::string StdStrStrMapToJsonString(
        const std::map<std::string, std::string>& args);
    std::string StdStrVecMapToJsonString(
        const std::map<std::string, std::vector<std::string>>& args);

    JavaObjectWeakGlobalRef weak_java_binance_native_worker_;
    binance::BinanceService* binance_service_;
    base::WeakPtrFactory<BinanceNativeWorker> weak_factory_;
};
}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_BINANCE_ANDROID_BINANCE_NATIVE_WORKER_H_
