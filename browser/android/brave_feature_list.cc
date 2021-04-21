/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/android/jni_string.h"
#include "base/feature_list.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/build/android/jni_headers/BraveFeatureList_jni.h"
#include "chrome/browser/about_flags.h"
#include "components/flags_ui/pref_service_flags_storage.h"

namespace chrome {
namespace android {

static void JNI_BraveFeatureList_EnableFeature(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& featureName,
    jboolean enabled,
    const base::android::JavaParamRef<jstring>& disabledValue) {
  std::string feature_name = ConvertJavaStringToUTF8(env, featureName);
  std::string disabled_value = ConvertJavaStringToUTF8(env, disabledValue);
  enabled ? feature_name += "@1" : feature_name += "@" + disabled_value;
  flags_ui::PrefServiceFlagsStorage flags_storage(
      g_brave_browser_process->local_state());
  about_flags::SetFeatureEntryEnabled(&flags_storage, feature_name, true);
}

}  // namespace android
}  // namespace chrome
