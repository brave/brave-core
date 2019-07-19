/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_shields_content_settings.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

#include "jni/BraveShieldsContentSettings_jni.h"

static const std::string kAllowResource = "allow";

namespace chrome {
namespace android {

BraveShieldsContentSettings* _brave_shields_content_settings = nullptr;

static void JNI_BraveShieldsContentSettings_Init(JNIEnv* env, const
    base::android::JavaParamRef<jobject>& jcaller) {
  _brave_shields_content_settings = new BraveShieldsContentSettings(env, jcaller);
}

BraveShieldsContentSettings::BraveShieldsContentSettings(JNIEnv* env, const base::android::JavaRef<jobject>& obj):
    weak_java_native_worker_(env, obj) {
  Java_BraveShieldsContentSettings_setNativePtr(env, obj, reinterpret_cast<intptr_t>(this));
}

BraveShieldsContentSettings::~BraveShieldsContentSettings() {
}

void BraveShieldsContentSettings::Destroy(JNIEnv* env, const
        base::android::JavaParamRef<jobject>& jcaller) {
  _brave_shields_content_settings = nullptr;
  delete this;
}

void BraveShieldsContentSettings::DispatchBlockedEventToJava(int tab_id, 
        const std::string& block_type, const std::string& subresource) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveShieldsContentSettings_blockedEvent(env, weak_java_native_worker_.get(env),
    tab_id, base::android::ConvertUTF8ToJavaString(env, block_type),
    base::android::ConvertUTF8ToJavaString(env, subresource));
}

// static
void BraveShieldsContentSettings::DispatchBlockedEvent(int tab_id, const std::string& block_type, 
  const std::string& subresource) {
  DCHECK(_brave_shields_content_settings);
  if (!_brave_shields_content_settings) {
    return;
  }
  _brave_shields_content_settings->DispatchBlockedEventToJava(tab_id, block_type, subresource);
}

base::android::ScopedJavaLocalRef<jstring> JNI_BraveShieldsContentSettings_GetShields(
    JNIEnv* env, jboolean incognito,
    const base::android::JavaParamRef<jstring>& host,
    const base::android::JavaParamRef<jstring>& resourceIndentifier) {
  HostContentSettingsMap* map;
  Profile* profile = ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
  if (incognito) {
    if (!profile->HasOffTheRecordProfile()) {
      // Allow shields there
      return base::android::ConvertUTF8ToJavaString(env, kAllowResource);
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

  return base::android::ConvertUTF8ToJavaString(env, setting_string);
}

void JNI_BraveShieldsContentSettings_SetShields(JNIEnv* env, jboolean incognito,
    const base::android::JavaParamRef<jstring>& host,
    const base::android::JavaParamRef<jstring>& resourceIndentifier,
    const base::android::JavaParamRef<jstring>& value) {
  GURL primary_url = GURL(base::android::ConvertJavaStringToUTF8(env, host));
  if (primary_url.is_empty()) {
    return;
  }
  ContentSetting setting;
  content_settings::ContentSettingFromString(base::android::ConvertJavaStringToUTF8(env, value), &setting);

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

}
}
