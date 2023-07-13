/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/build/android/jni_headers/DataFilesComponentInstaller_jni.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"

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
            Java_DataFilesComponentInstaller_onRegisterAndInstallDone(
                env, java_callback);
          },
          env, std::move(java_callback)));
}

}  // namespace

static void JNI_DataFilesComponentInstaller_RegisterAndInstall(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& callback) {
  component_updater::ComponentUpdateService* cus =
      g_browser_process->component_updater();

  base::android::ScopedJavaGlobalRef<jobject> java_callback;
  java_callback.Reset(env, callback);

  ::brave_wallet::RegisterWalletDataFilesComponentOnDemand(
      cus, base::BindOnce(&NativeRegisterAndInstallCallback, env,
                          std::move(java_callback),
                          base::SequencedTaskRunner::GetCurrentDefault()));
}

}  // namespace android
}  // namespace chrome
