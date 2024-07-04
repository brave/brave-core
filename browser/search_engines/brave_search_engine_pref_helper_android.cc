/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/android/java/org/chromium/chrome/browser/search_engines/jni_headers/BraveSearchEnginePrefHelper_jni.h"

#include "brave/components/brave_search/browser/prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

namespace {
Profile* GetOriginalProfile() {
  return ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
}
}  // namespace

void JNI_BraveSearchEnginePrefHelper_SetFetchSEFromNative(JNIEnv* env,
                                                          jboolean value) {
  GetOriginalProfile()->GetPrefs()->SetBoolean(
      brave_search::prefs::kFetchFromNative, value);
}

jboolean JNI_BraveSearchEnginePrefHelper_GetFetchSEFromNative(JNIEnv* env) {
  return GetOriginalProfile()->GetPrefs()->GetBoolean(
      brave_search::prefs::kFetchFromNative);
}
