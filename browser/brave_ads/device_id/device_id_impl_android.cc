/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/device_id/device_id_impl.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/functional/callback.h"
#include "brave/browser/brave_ads/android/jni_headers/DeviceIdImplAndroid_jni.h"

namespace brave_ads {

// static
void DeviceIdImpl::GetRawDeviceId(DeviceIdCallback callback) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> android_id =
      Java_DeviceIdImplAndroid_getAndroidId(env);
  if (!android_id) {
    return std::move(callback).Run({});
  }

  std::string device_id =
      base::android::ConvertJavaStringToUTF8(env, android_id.obj());
  std::move(callback).Run(std::move(device_id));
}

}  // namespace brave_ads
