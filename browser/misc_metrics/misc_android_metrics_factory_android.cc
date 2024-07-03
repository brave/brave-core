/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/misc_android_metrics.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/build/android/jni_headers/MiscAndroidMetricsFactory_jni.h"
#include "chrome/browser/profiles/profile.h"

namespace chrome {
namespace android {
static jlong JNI_MiscAndroidMetricsFactory_GetInterfaceToMiscAndroidMetrics(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  auto pending = misc_metrics::ProfileMiscMetricsServiceFactory::GetInstance()
                     ->GetServiceForContext(profile)
                     ->GetMiscAndroidMetrics()
                     ->MakeRemote();

  return static_cast<jlong>(pending.PassPipe().release().value());
}

}  // namespace android
}  // namespace chrome
