/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"

#include "base/base_jni/BraveBundleUtils_jni.h"
#include "base/base_jni/BundleUtils_jni.h"

namespace {

// We need this just to avoid unused function
// 'Java_BundleUtils_getNativeLibraryPath' error message.
bool DummyBundleUtils() {
  base::android::ScopedJavaLocalRef<jstring> java_path =
      Java_BundleUtils_getNativeLibraryPath(
          nullptr, base::android::ScopedJavaLocalRef<jstring>(),
          base::android::ScopedJavaLocalRef<jstring>());
  if (!java_path) {
    return false;
  }
  return DummyBundleUtils();
}

}  // namespace

#define Java_BundleUtils_getNativeLibraryPath \
  Java_BraveBundleUtils_getNativeLibraryPathTrySplitAbi

#include "src/base/android/bundle_utils.cc"

#undef Java_BundleUtils_getNativeLibraryPath
