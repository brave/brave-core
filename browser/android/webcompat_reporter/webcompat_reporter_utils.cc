/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/logging.h"
#include "brave/build/android/jni_headers/BraveWebcompatReporterUtils_jni.h"
#include "brave/common/brave_channel_info.h"
#include "brave/components/version_info/version_info.h"

namespace webcompat_reporter {

static std::string JNI_BraveWebcompatReporterUtils_GetChannel(JNIEnv* env) {
  return brave::GetChannelName();
}

static std::string JNI_BraveWebcompatReporterUtils_GetBraveVersion(
    JNIEnv* env) {
  return version_info::GetBraveVersionWithoutChromiumMajorVersion();
}

}  // namespace webcompat_reporter
