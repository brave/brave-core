/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_BRAVE_SYNC_STORAGE_H_
#define BRAVE_BROWSER_ANDROID_BRAVE_SYNC_STORAGE_H_

#include <jni.h>
#include "base/android/jni_weak_ref.h"

namespace chrome {
namespace android {

class BraveSyncWorker {
 public:
  BraveSyncWorker(JNIEnv* env, jobject obj);
  ~BraveSyncWorker();

 private:
  JavaObjectWeakGlobalRef weak_java_shields_config_;
};

}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_ANDROID_BRAVE_SYNC_STORAGE_H_
