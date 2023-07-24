/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/component_updater/brave_component_updater_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/json/json_writer.h"
#include "brave/build/android/jni_headers/BraveComponentUpdater_jni.h"
#include "chrome/browser/browser_process.h"
#include "components/update_client/crx_update_item.h"
#include "components/update_client/update_engine.h"

namespace chrome {
namespace android {

BraveComponentUpdaterAndroid::BraveComponentUpdaterAndroid(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj)
    : weak_java_brave_sync_worker_(env, obj) {
  Java_BraveComponentUpdater_setNativePtr(env, obj,
                                          reinterpret_cast<intptr_t>(this));

  component_updater::ComponentUpdateService* component_updater =
      g_browser_process->component_updater();
  DCHECK(component_updater);

  if (component_updater) {
    observation_.Observe(component_updater);
  }
}

BraveComponentUpdaterAndroid::~BraveComponentUpdaterAndroid() {
  // Observer will be removed by ScopedObservation
}

void BraveComponentUpdaterAndroid::Destroy(JNIEnv* env) {
  delete this;
}

void BraveComponentUpdaterAndroid::OnEvent(Events event,
                                           const std::string& id) {
  // Notify Java code
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveComponentUpdater_componentStateUpdated(
      env, weak_java_brave_sync_worker_.get(env), static_cast<int>(event),
      base::android::ConvertUTF8ToJavaString(env, id));
}

base::android::ScopedJavaLocalRef<jstring>
BraveComponentUpdaterAndroid::GetUpdateState(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& id) {
  std::string json_string;
  update_client::CrxUpdateItem item;

  if (g_browser_process->component_updater()->GetComponentDetails(
          base::android::ConvertJavaStringToUTF8(id), &item)) {
    base::Value::Dict value;
    value.Set("id", item.id);
    value.Set("downloaded_bytes", static_cast<double>(item.downloaded_bytes));
    value.Set("total_bytes", static_cast<double>(item.total_bytes));
    value.Set("state", static_cast<int>(item.state));

    if (!base::JSONWriter::Write(value, &json_string)) {
      VLOG(1) << "Writing to JSON string failed. Passing empty result to Java "
                 "code.";
    }
  }
  return base::android::ConvertUTF8ToJavaString(env, json_string);
}

static void JNI_BraveComponentUpdater_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new BraveComponentUpdaterAndroid(env, jcaller);
}

}  // namespace android
}  // namespace chrome
