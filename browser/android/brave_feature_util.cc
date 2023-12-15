/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/android/jni_string.h"
#include "base/feature_list.h"
#include "brave/build/android/jni_headers/BraveFeatureUtil_jni.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/browser_process.h"
#include "components/flags_ui/feature_entry.h"
#include "components/flags_ui/pref_service_flags_storage.h"

namespace chrome {
namespace android {

int GetNumberOfOptions(const std::string& internal_name) {
  DCHECK(about_flags::GetCurrentFlagsState());
  if (!about_flags::GetCurrentFlagsState()) {
    return 0;
  }
  const flags_ui::FeatureEntry* entry =
      about_flags::GetCurrentFlagsState()->FindFeatureEntryByName(
          internal_name);
  DCHECK(entry);
  if (!entry) {
    return 0;
  }
  return entry->NumOptions();
}

static void JNI_BraveFeatureUtil_EnableFeature(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& featureName,
    jboolean enabled,
    jboolean fallback_to_default) {
  std::string feature_name =
      base::android::ConvertJavaStringToUTF8(env, featureName);
  std::string disabled_value =
      fallback_to_default
          ? "0"
          : std::to_string(GetNumberOfOptions(feature_name) - 1);
  enabled ? feature_name += "@1" : feature_name += "@" + disabled_value;
  flags_ui::PrefServiceFlagsStorage flags_storage(
      g_browser_process->local_state());
  about_flags::SetFeatureEntryEnabled(&flags_storage, feature_name, true);
}

}  // namespace android
}  // namespace chrome
