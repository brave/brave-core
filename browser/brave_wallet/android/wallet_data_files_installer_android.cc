/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/build/android/jni_headers/WalletDataFilesInstaller_jni.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "components/component_updater/component_updater_service.h"

static_assert BUILDFLAG(IS_ANDROID);

namespace chrome {
namespace android {

namespace {
void NativeRegisterAndInstallCallback(
    JNIEnv* env,
    base::android::ScopedJavaGlobalRef<jobject> java_callback,
    scoped_refptr<base::SequencedTaskRunner> post_response_runner) {
  post_response_runner->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](JNIEnv* env,
             base::android::ScopedJavaGlobalRef<jobject> java_callback) {
            Java_WalletDataFilesInstaller_onRegisterAndInstallDone(
                env, java_callback);
          },
          env, std::move(java_callback)));
}

bool IsBraveWalletDataFilesComponentRegistered() {
  std::vector<std::string> registered_ids =
      g_browser_process->component_updater()->GetComponentIDs();
  return base::Contains(
      registered_ids,
      std::string(brave_wallet::GetWalletDataFilesComponentId()));
}

}  // namespace

static void
JNI_WalletDataFilesInstaller_RegisterWalletDataFilesComponentOnDemand(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& callback) {
  if (IsBraveWalletDataFilesComponentRegistered()) {
    // In anyway will be removed
    Java_WalletDataFilesInstaller_onRegisterAndInstallDone(env, callback);
    return;
  }

  component_updater::ComponentUpdateService* cus =
      g_browser_process->component_updater();

  base::android::ScopedJavaGlobalRef<jobject> java_callback;
  java_callback.Reset(env, callback);

  ::brave_wallet::RegisterWalletDataFilesComponentOnDemand(
      cus, base::BindOnce(&NativeRegisterAndInstallCallback, env,
                          std::move(java_callback),
                          base::SequencedTaskRunner::GetCurrentDefault()));
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_WalletDataFilesInstaller_GetWalletDataFilesComponentId(JNIEnv* env) {
  return base::android::ConvertUTF8ToJavaString(
      env, ::brave_wallet::GetWalletDataFilesComponentId());
}

}  // namespace android
}  // namespace chrome
