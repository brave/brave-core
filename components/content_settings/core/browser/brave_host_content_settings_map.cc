/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"

#include "brave/components/content_settings/core/browser/brave_content_settings_ephemeral_provider.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"

using content_settings::UserModifiableProvider;

void BraveHostContentSettingsMap::ClearSettingsForPluginsType(
    bool is_shields) {
  if (is_shields) {
    static_cast<content_settings::BravePrefProvider*>(GetPrefProvider())->
        ClearAllShieldsContentSettings();
    FlushLossyWebsiteSettings();
  } else {
    static_cast<content_settings::BraveEphemeralProvider*>(
        GetEphemeralProvider())->ClearFlashPluginContentSettings();
  }
}

void BraveHostContentSettingsMap::ClearSettingsForPluginsType(
    base::Time begin_time,
    base::Time end_time,
    bool is_shields) {
  if (begin_time.is_null() && (end_time.is_null() || end_time.is_max())) {
      ClearSettingsForPluginsType(is_shields);
    return;
  }

  // Target resource ids from plugin types for clearing.
  std::vector<std::string> resource_ids;
  if (is_shields) {
    resource_ids = content_settings::GetShieldsResourceIDs();
  } else {
    // Empty string is flash resource id.
    resource_ids.push_back("");
  }

  auto* provider = is_shields ? GetPrefProvider() : GetEphemeralProvider();
  for (const auto& resource_id : resource_ids) {
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

content_settings::UserModifiableProvider*
BraveHostContentSettingsMap::GetEphemeralProvider() {
  UsedContentSettingsProviders();
  return static_cast<UserModifiableProvider*>(
      content_settings_providers_[EPHEMERAL_PROVIDER].get());
}
