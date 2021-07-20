/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/android/brave_wallet_native_worker.h"

#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/brave_wallet/asset_ratio_controller_factory.h"
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/build/android/jni_headers/BraveWalletNativeWorker_jni.h"
#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace chrome {
namespace android {

BraveWalletNativeWorker::BraveWalletNativeWorker(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj)
    : weak_java_brave_wallet_native_worker_(env, obj), weak_ptr_factory_(this) {
  Java_BraveWalletNativeWorker_setNativePtr(env, obj,
                                            reinterpret_cast<intptr_t>(this));
}

BraveWalletNativeWorker::~BraveWalletNativeWorker() {}

void BraveWalletNativeWorker::EnsureConnected() {
  auto* profile = ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
  if (!keyring_controller_) {
    auto pending =
        brave_wallet::KeyringControllerFactory::GetInstance()->GetForContext(
            profile);
    keyring_controller_.Bind(std::move(pending));
  }
  DCHECK(keyring_controller_);
  keyring_controller_.set_disconnect_handler(
      base::BindOnce(&BraveWalletNativeWorker::OnConnectionError,
                     weak_ptr_factory_.GetWeakPtr()));

  if (!asset_ratio_controller_) {
    auto pending =
        brave_wallet::AssetRatioControllerFactory::GetInstance()->GetForContext(
            profile);
    asset_ratio_controller_.Bind(std::move(pending));
  }
  DCHECK(asset_ratio_controller_);
  asset_ratio_controller_.set_disconnect_handler(
      base::BindOnce(&BraveWalletNativeWorker::OnConnectionError,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletNativeWorker::OnConnectionError() {
  keyring_controller_.reset();
  asset_ratio_controller_.reset();
  EnsureConnected();
}

void BraveWalletNativeWorker::Destroy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  delete this;
}

base::android::ScopedJavaLocalRef<jstring>
BraveWalletNativeWorker::GetRecoveryWords(JNIEnv* env) {
  EnsureConnected();

  auto callback = base::BindOnce([](const std::string& mnemonic) {});
  keyring_controller_->GetMnemonicForDefaultKeyring(std::move(callback));

  // TODO(bridiver) - convert this to callback
  return base::android::ConvertUTF8ToJavaString(env, "");
}

bool BraveWalletNativeWorker::IsWalletLocked(JNIEnv* env) {
  EnsureConnected();
  // return keyring_controller_->IsLocked();
  return false;
}

base::android::ScopedJavaLocalRef<jstring>
BraveWalletNativeWorker::CreateWallet(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& password) {
  EnsureConnected();

  auto callback = base::BindOnce([](const std::string& mnemonic) {});
  keyring_controller_->CreateWallet(
      base::android::ConvertJavaStringToUTF8(env, password),
      std::move(callback));

  // TODO(bridiver) - convert this to callback
  return base::android::ConvertUTF8ToJavaString(env, "");
}

void BraveWalletNativeWorker::LockWallet(JNIEnv* env) {
  EnsureConnected();
  keyring_controller_->Lock();
}

bool BraveWalletNativeWorker::UnlockWallet(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& password) {
  EnsureConnected();

  auto callback = base::BindOnce([](bool unlocked) {});
  keyring_controller_->Unlock(
      base::android::ConvertJavaStringToUTF8(env, password),
      std::move(callback));

  // TODO(bridiver) - convert to callback
  return true;
}

base::android::ScopedJavaLocalRef<jstring>
BraveWalletNativeWorker::RestoreWallet(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& mnemonic,
    const base::android::JavaParamRef<jstring>& password) {
  EnsureConnected();
  auto callback = base::BindOnce([](bool is_valid) {});
  keyring_controller_->RestoreWallet(
      base::android::ConvertJavaStringToUTF8(env, mnemonic),
      base::android::ConvertJavaStringToUTF8(env, password),
      std::move(callback));

  // TODO(bridiver) - convert to callback and this should return true/false not
  // the mnemonic
  return base::android::ConvertUTF8ToJavaString(env, "");
}

void BraveWalletNativeWorker::ResetWallet(JNIEnv* env) {
  EnsureConnected();
  keyring_controller_->Reset();
}

void BraveWalletNativeWorker::GetAssetPrice(
    JNIEnv* env,
    const base::android::JavaParamRef<jobjectArray>& from_assets,
    const base::android::JavaParamRef<jobjectArray>& to_assets) {
  EnsureConnected();

  std::vector<std::string> assets_from;
  base::android::AppendJavaStringArrayToStringVector(env, from_assets,
                                                     &assets_from);
  std::vector<std::string> assets_to;
  base::android::AppendJavaStringArrayToStringVector(env, to_assets,

  asset_ratio_controller_->GetPrice(
      assets_from, assets_to,
      base::BindOnce(&BraveWalletNativeWorker::OnGetPrice,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletNativeWorker::OnGetPrice(
    bool success,
    std::vector<brave_wallet::mojom::AssetPricePtr> prices) {
  std::string prices_json;
  base::Value list(base::Value::Type::LIST);
  for (const auto& asset_price : prices) {
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey("from_asset", asset_price->from_asset);
    dict.SetStringKey("to_asset", asset_price->to_asset);
    dict.SetStringKey("price", asset_price->price);
    dict.SetStringKey("asset_24h_change", asset_price->asset_24h_change);
    list.Append(std::move(dict));
  }
  base::JSONWriter::Write(list, &prices_json);

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveWalletNativeWorker_OnGetPrice(
      env, weak_java_brave_wallet_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, prices_json), success);
}

void BraveWalletNativeWorker::GetAssetPriceHistory(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& asset,
    const jint timeFrameType) {
  EnsureConnected();

  brave_wallet::mojom::AssetPriceTimeframe time_frame;
  if (timeFrameType == 0) {
    time_frame = brave_wallet::mojom::AssetPriceTimeframe::Live;
  } else if (timeFrameType == 1) {
    time_frame = brave_wallet::mojom::AssetPriceTimeframe::OneDay;
  } else if (timeFrameType == 2) {
    time_frame = brave_wallet::mojom::AssetPriceTimeframe::OneWeek;
  } else if (timeFrameType == 3) {
    time_frame = brave_wallet::mojom::AssetPriceTimeframe::OneMonth;
  } else if (timeFrameType == 4) {
    time_frame = brave_wallet::mojom::AssetPriceTimeframe::ThreeMonths;
  } else if (timeFrameType == 5) {
    time_frame = brave_wallet::mojom::AssetPriceTimeframe::OneYear;
  } else {
    time_frame = brave_wallet::mojom::AssetPriceTimeframe::All;
  }

  asset_ratio_controller_->GetPriceHistory(
      base::android::ConvertJavaStringToUTF8(env, asset), time_frame,
      base::BindOnce(&BraveWalletNativeWorker::OnGetPriceHistory,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletNativeWorker::OnGetPriceHistory(
    bool success,
    std::vector<brave_wallet::mojom::AssetTimePricePtr> values) {
  std::string price_history;
  base::Value list(base::Value::Type::LIST);
  for (const auto& asset_time_price : values) {
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetDoubleKey("time", (asset_time_price->date).ToDoubleT());
    dict.SetStringKey("price", asset_time_price->price);
    list.Append(std::move(dict));
  }
  base::JSONWriter::Write(list, &price_history);

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveWalletNativeWorker_OnGetPriceHistory(
      env, weak_java_brave_wallet_native_worker_.get(env),
      base::android::ConvertUTF8ToJavaString(env, price_history), success);
}

static void JNI_BraveWalletNativeWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new BraveWalletNativeWorker(env, jcaller);
}

}  // namespace android
}  // namespace chrome
