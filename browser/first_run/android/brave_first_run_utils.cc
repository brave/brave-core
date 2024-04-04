/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <jni.h>

#include "brave/browser/first_run/first_run.h"
#include "brave/build/android/jni_headers/BraveFirstRunUtils_jni.h"

static jboolean JNI_BraveFirstRunUtils_IsMetricsReportingOptIn(JNIEnv* env) {
  return brave::first_run::IsMetricsReportingOptIn();
}
