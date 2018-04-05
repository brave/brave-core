/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"

#include "components/content_settings/core/common/content_settings_pattern.h"

BraveHostContentSettingsMap::BraveHostContentSettingsMap(
    PrefService* prefs,
    bool is_incognito_profile,
    bool is_guest_profile,
    bool store_last_modified)
    : HostContentSettingsMap(prefs, is_incognito_profile, is_guest_profile,
        store_last_modified) {
  InitializeFingerprintingContentSetting();
}

BraveHostContentSettingsMap::~BraveHostContentSettingsMap() {
}

void BraveHostContentSettingsMap::InitializeFingerprintingContentSetting() {
  SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS,
      "fingerprinting",
      CONTENT_SETTING_ALLOW);
}
