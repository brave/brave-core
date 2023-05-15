/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "base/logging.h"

// Prevent include of "android_webview/lib/webview_jni_onload.h"
// We want to make it to re-define android_webview::OnJNIOnLoadInit()
#define ANDROID_WEBVIEW_LIB_WEBVIEW_JNI_ONLOAD_H_

namespace android_webview {

bool OnJNIOnLoadInit() {
  CHECK(false) << "Android Webview API must not be invoked";
  return false;
}

}  // namespace android_webview

#include "src/chrome/browser/android/monochrome_entry_point.cc"
