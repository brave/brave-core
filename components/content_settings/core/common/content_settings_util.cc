/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/common/content_settings_util.h"

#include <map>
#include <vector>

#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "components/content_settings/core/common/content_settings_types.h"

namespace content_settings {

ContentSetting GetDefaultFromContentSettingsType(
    const ContentSettingsType& content_type,
    const GURL& primary_url,
    const GURL& secondary_url) {
  if (content_type == ContentSettingsType::ADS) {
    return CONTENT_SETTING_BLOCK;
  } else if (content_type == ContentSettingsType::BRAVE_COSMETIC_FILTERING) {
    return secondary_url == GURL("https://firstParty/")
        ? CONTENT_SETTING_ALLOW
        : CONTENT_SETTING_BLOCK;
  } else if (content_type == ContentSettingsType::BRAVE_TRACKERS) {
    return CONTENT_SETTING_BLOCK;
  } else if (content_type ==
             ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES) {
    return CONTENT_SETTING_BLOCK;
  } else if (content_type == ContentSettingsType::BRAVE_SHIELDS) {
    return CONTENT_SETTING_ALLOW;
  } else if (content_type == ContentSettingsType::BRAVE_REFERRERS) {
    return CONTENT_SETTING_BLOCK;
  } else if (content_type == ContentSettingsType::BRAVE_COOKIES) {
    return secondary_url == GURL("https://firstParty/") ? CONTENT_SETTING_BLOCK
                                                        : CONTENT_SETTING_ALLOW;
  } else if (content_type == ContentSettingsType::BRAVE_SPEEDREADER) {
    return CONTENT_SETTING_ALLOW;
  } else if (content_type == ContentSettingsType::BRAVE_ETHEREUM) {
    return CONTENT_SETTING_DEFAULT;
  }

  return CONTENT_SETTING_BLOCK;
}

bool IsAllowContentSetting(const ContentSettingsForOneType& content_settings,
                           const GURL& primary_url,
                           const GURL& secondary_url,
                           const ContentSettingsType& content_type) {
  ContentSetting setting = GetDefaultFromContentSettingsType(
      content_type, primary_url, secondary_url);

  for (const auto& entry : content_settings) {
    if (entry.primary_pattern.Matches(primary_url) &&
        entry.secondary_pattern.Matches(secondary_url)) {
      setting = entry.GetContentSetting();
      break;
    }
  }

  return setting == CONTENT_SETTING_ALLOW;
}

}  // namespace content_settings
