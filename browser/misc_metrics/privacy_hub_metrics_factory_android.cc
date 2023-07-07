/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/build/android/jni_headers/PrivacyHubMetricsFactory_jni.h"
#include "brave/components/misc_metrics/privacy_hub_metrics.h"

namespace chrome {
namespace android {
static jlong JNI_PrivacyHubMetricsFactory_GetInterfaceToPrivacyHubMetrics(
    JNIEnv* env) {
  auto pending = g_brave_browser_process->process_misc_metrics()
                     ->privacy_hub_metrics()
                     ->MakeRemote();

  return static_cast<jlong>(pending.PassPipe().release().value());
}

}  // namespace android
}  // namespace chrome
