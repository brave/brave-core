/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"

#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/prefs/pref_service.h"

BraveHostContentSettingsMap::BraveHostContentSettingsMap(
    PrefService* prefs,
    bool is_incognito_profile,
    bool is_guest_profile,
    bool store_last_modified,
    bool migrate_requesting_and_top_level_origin_settings)
    : HostContentSettingsMap(prefs, is_incognito_profile, is_guest_profile,
        store_last_modified, migrate_requesting_and_top_level_origin_settings) {
  InitializeCookieContentSetting();
  InitializeFlashContentSetting();
}

BraveHostContentSettingsMap::~BraveHostContentSettingsMap() {
}

void BraveHostContentSettingsMap::InitializeCookieContentSetting() {
  // We intentionally do not use the cookies content settings so that
  // these special rules do not show up in Chromium UI.
  SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kCookies,
      CONTENT_SETTING_ALLOW);
}

void BraveHostContentSettingsMap::InitializeFlashContentSetting() {
  SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS,
      // One would think this should be brave_shields::kFlash; however, if you
      // use it, it will always ask and click-to-play will not work.
      std::string(),
      CONTENT_SETTING_BLOCK);
}
