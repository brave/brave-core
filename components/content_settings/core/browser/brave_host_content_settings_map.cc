/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"

base::Time BraveHostContentSettingsMap::GetShieldsSettingLastModifiedDate(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    const std::string& resource_identifier) const {
  return GetPrefProvider()->GetWebsiteSettingLastModified(
      primary_pattern, secondary_pattern,
      ContentSettingsType::PLUGINS, resource_identifier);
}

void BraveHostContentSettingsMap::ClearSettingsForPluginsType() {
  static_cast<content_settings::BravePrefProvider*>(GetPrefProvider())->
      ClearAllShieldsContentSettings();
  FlushLossyWebsiteSettings();
}

void BraveHostContentSettingsMap::ClearSettingsForPluginsType(
    base::Time begin_time,
    base::Time end_time) {
  if (begin_time.is_null() && (end_time.is_null() || end_time.is_max())) {
      ClearSettingsForPluginsType();
    return;
  }

  auto* provider = GetPrefProvider();
  for (const auto& resource_id : content_settings::GetShieldsResourceIDs()) {
    ContentSettingsForOneType settings;
    ContentSettingsType content_type = ContentSettingsType::PLUGINS;
    GetSettingsForOneType(content_type, resource_id, &settings);
    for (const ContentSettingPatternSource& setting : settings) {
      base::Time last_modified = provider->GetWebsiteSettingLastModified(
          setting.primary_pattern, setting.secondary_pattern, content_type,
          resource_id);
      if (last_modified >= begin_time &&
          (last_modified < end_time || end_time.is_null())) {
        provider->SetWebsiteSetting(setting.primary_pattern,
                                    setting.secondary_pattern, content_type,
                                    resource_id, nullptr);
      }
    }
  }
}
