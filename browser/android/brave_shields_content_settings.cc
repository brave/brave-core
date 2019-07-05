/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

#include "jni/BraveShieldsContentSettings_jni.h"

static const std::string kBlockResource = "block";
static const std::string kAllowResource = "allow";

jboolean JNI_BraveShieldsContentSettings_GetShields(JNIEnv* env, jboolean incognito,
    const base::android::JavaParamRef<jstring>& host,
    const base::android::JavaParamRef<jstring>& resourceIndentifier) {
  HostContentSettingsMap* map;
  Profile* profile = ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
  if (incognito) {
    if (!profile->HasOffTheRecordProfile()) {
      // Allow shields there
      return true;
    }
    map = HostContentSettingsMapFactory::GetForProfile(
        profile->GetOffTheRecordProfile());
  } else {
    map = HostContentSettingsMapFactory::GetForProfile(profile);
  }

  ContentSetting setting = map->GetContentSetting(
      GURL(base::android::ConvertJavaStringToUTF8(env, host)), GURL(""), 
      CONTENT_SETTINGS_TYPE_PLUGINS, 
      base::android::ConvertJavaStringToUTF8(env, resourceIndentifier));

  std::string setting_string =
      content_settings::ContentSettingToString(setting);
  DCHECK(!setting_string.empty());

  return setting_string != kAllowResource;
}

void JNI_BraveShieldsContentSettings_SetShields(JNIEnv* env, jboolean incognito,
    const base::android::JavaParamRef<jstring>& host,
    const base::android::JavaParamRef<jstring>& resourceIndentifier,
    jboolean value) {
  GURL primary_url = GURL(base::android::ConvertJavaStringToUTF8(env, host));
  if (primary_url.is_empty()) {
    return;
  }
  std::string setting_string = (value ? kBlockResource : kAllowResource);
  ContentSetting setting;
  content_settings::ContentSettingFromString(setting_string, &setting);

  HostContentSettingsMap* map;
  Profile* profile = ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
  if (incognito) {
    if (!profile->HasOffTheRecordProfile()) {
      // Do nothing here
      return;
    }
    map = HostContentSettingsMapFactory::GetForProfile(
        profile->GetOffTheRecordProfile());
  } else {
    map = HostContentSettingsMapFactory::GetForProfile(profile);
  }
  map->SetContentSettingDefaultScope(
        GURL(base::android::ConvertJavaStringToUTF8(env, host)), GURL(""),
        CONTENT_SETTINGS_TYPE_PLUGINS, 
        base::android::ConvertJavaStringToUTF8(env, resourceIndentifier), setting);
}
