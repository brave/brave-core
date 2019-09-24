/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/build/android/jni_headers/DesktopModePreferences_jni.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "components/prefs/pref_service.h"

namespace chrome {
namespace android {

void JNI_DesktopModePreferences_SetDesktopModeEnabled(
    JNIEnv* env,
    jboolean enabled,
    const base::android::JavaParamRef<jobject>& j_profile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  return profile->GetPrefs()->SetBoolean(kDesktopModeEnabled, enabled);
}

jboolean JNI_DesktopModePreferences_GetDesktopModeEnabled(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  return profile->GetPrefs()->GetBoolean(kDesktopModeEnabled);
}

}  // namespace android
}  // namespace chrome
