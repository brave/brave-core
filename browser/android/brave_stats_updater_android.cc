/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/android/jni_android.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/brave_stats_updater.h"

#include "jni/BraveActivity_jni.h"

namespace chrome {
namespace android {

static void JNI_BraveActivity_RestartStatsUpdater(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  g_brave_browser_process->brave_stats_updater()->Stop();
  g_brave_browser_process->brave_stats_updater()->Start();
}

}  // namespace android
}  // namespace chrome
