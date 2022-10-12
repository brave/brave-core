/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/android/java/org/chromium/chrome/browser/search_engines/settings/jni_headers/BraveSearchEnginePrefHelper_jni.h"

#include "base/android/jni_string.h"
#include "brave/components/brave_search/browser/prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"

namespace {
Profile* GetOriginalProfile() {
  return ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
}
/*
const char kSyncedDefaultPrivateSearchProviderGUID[] =
    "brave.default_private_search_provider_guid";

*/
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

void JNI_BraveSearchEnginePrefHelper_SetPrivateSEGuid(
  JNIEnv* env,
  const base::android::JavaParamRef<jstring>& private_se_guid) {
    std::string str_private_se_guid =
        base::android::ConvertJavaStringToUTF8(private_se_guid);
LOG(ERROR) << "[DSE] " << __func__ << " str_private_se_guid="<<str_private_se_guid;      
  GetOriginalProfile()->GetPrefs()->SetString(
      prefs::kSyncedDefaultPrivateSearchProviderGUID, str_private_se_guid);
}

base::android::ScopedJavaLocalRef<jstring> JNI_BraveSearchEnginePrefHelper_GetPrivateSEGuid(
  JNIEnv* env) {
  std::string str_private_se_guid = GetOriginalProfile()->GetPrefs()->GetString(
      prefs::kSyncedDefaultPrivateSearchProviderGUID);
LOG(ERROR) << "[DSE] " << __func__ << " str_private_se_guid="<<str_private_se_guid;      
  return base::android::ConvertUTF8ToJavaString(env, str_private_se_guid);
}
