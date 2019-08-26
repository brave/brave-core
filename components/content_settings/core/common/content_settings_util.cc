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

ContentSetting GetDefaultFromResourceIdentifier(
    const std::string& resource_identifier,
    const GURL& primary_url,
    const GURL& secondary_url) {
  if (resource_identifier == brave_shields::kAds) {
    return CONTENT_SETTING_BLOCK;
  } else if (resource_identifier == brave_shields::kTrackers) {
    return CONTENT_SETTING_BLOCK;
  } else if (resource_identifier == brave_shields::kHTTPUpgradableResources) {
    return CONTENT_SETTING_BLOCK;
  } else if (resource_identifier == brave_shields::kBraveShields) {
    return CONTENT_SETTING_ALLOW;
  } else if (resource_identifier == brave_shields::kReferrers) {
    return CONTENT_SETTING_BLOCK;
  } else if (resource_identifier == brave_shields::kCookies) {
    return secondary_url == GURL("https://firstParty/")
        ? CONTENT_SETTING_BLOCK
        : CONTENT_SETTING_ALLOW;
  }
  return CONTENT_SETTING_BLOCK;
}

bool IsAllowContentSetting(const ContentSettingsForOneType& content_settings,
                           const GURL& primary_url,
                           const GURL& secondary_url,
                           const std::string& resource_identifier) {
  ContentSetting setting =
      GetDefaultFromResourceIdentifier(resource_identifier,
                                       primary_url,
                                       secondary_url);

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
