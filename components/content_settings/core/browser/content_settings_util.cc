/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/content_settings_util.h"

#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
// #include "components/content_settings/core/common/content_settings_utils.h"

namespace content_settings {

bool GetDefaultFromResourceIdentifier(const std::string& resource_identifier,
                                      const GURL& primary_url,
                                      const GURL& secondary_url) {
  if (resource_identifier == brave_shields::kAds) {
    return false;
  } else if (resource_identifier == brave_shields::kTrackers) {
    return false;
  } else if (resource_identifier == brave_shields::kHTTPUpgradableResources) {
    return false;
  } else if (resource_identifier == brave_shields::kBraveShields) {
    return true;
  } else if (resource_identifier == brave_shields::kReferrers) {
    return false;
  } else if (resource_identifier == brave_shields::kCookies) {
    return secondary_url == GURL("https://firstParty/");
  }
  return false;
}

bool IsAllowContentSetting(HostContentSettingsMap* content_settings,
                           const GURL& primary_url,
                           const GURL& secondary_url,
                           ContentSettingsType setting_type,
                           const std::string& resource_identifier) {
  ContentSetting setting = content_settings->GetContentSetting(
      primary_url,
      secondary_url,
      setting_type,
      resource_identifier);

  // TODO(bbondy): Add a static RegisterUserPrefs method for shields and use
  // prefs instead of simply returning true / false below.
  if (setting == CONTENT_SETTING_DEFAULT) {
    return GetDefaultFromResourceIdentifier(resource_identifier,
                                            primary_url,
                                            secondary_url);
  }
  return setting == CONTENT_SETTING_ALLOW;
}

}  // namespace content_settings
