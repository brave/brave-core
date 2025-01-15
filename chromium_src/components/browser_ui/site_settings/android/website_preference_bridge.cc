/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/strcat.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_manager.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "url/url_constants.h"

// Must come after all headers that specialize FromJniType() / ToJniType().
#include "components/browser_ui/site_settings/android/site_settings_jni_headers/WebsitePreferenceBridge_jni.h"

// Default is ALLOW
#define BACKGROUND_SYNC \
  AUTOPLAY:             \
  case ContentSettingsType::BACKGROUND_SYNC

// Default is ASK
#define CLIPBOARD_READ_WRITE                        \
  BRAVE_GOOGLE_SIGN_IN:                             \
  case ContentSettingsType::BRAVE_LOCALHOST_ACCESS: \
  case ContentSettingsType::CLIPBOARD_READ_WRITE

#define JNI_WebsitePreferenceBridge_ClearCookieData \
  JNI_WebsitePreferenceBridge_ClearCookieData_ChromiumImpl

#include "src/components/browser_ui/site_settings/android/website_preference_bridge.cc"

#undef BACKGROUND_SYNC
#undef CLIPBOARD_READ_WRITE
#undef JNI_WebsitePreferenceBridge_ClearCookieData

static void JNI_WebsitePreferenceBridge_ClearCookieData(
    JNIEnv* env,
    const JavaParamRef<jobject>& jbrowser_context_handle,
    const JavaParamRef<jstring>& jorigin) {
  JNI_WebsitePreferenceBridge_ClearCookieData_ChromiumImpl(
      env, jbrowser_context_handle, jorigin);

  BrowserContext* browser_context = unwrap(jbrowser_context_handle);
  const GURL url(ConvertJavaStringToUTF8(env, jorigin));

  if (url.is_valid()) {
    auto* settings_map = GetHostContentSettingsMap(browser_context);
    settings_map->SetWebsiteSettingDefaultScope(
        url, url, ContentSettingsType::BRAVE_SHIELDS_METADATA, base::Value());

    // Clear both http and https on Android. For some reason, Android may pass
    // http origin into this function even if the site is using https.
    const bool is_http = url.SchemeIs(url::kHttpScheme);
    const GURL additional_url(
        base::StrCat({is_http ? url::kHttpsScheme : url::kHttpScheme,
                      url::kStandardSchemeSeparator, url.host_piece()}));
    settings_map->SetWebsiteSettingDefaultScope(
        additional_url, additional_url,
        ContentSettingsType::BRAVE_SHIELDS_METADATA, base::Value());
  }
}
