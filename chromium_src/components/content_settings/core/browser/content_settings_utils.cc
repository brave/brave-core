/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/browser/content_settings_utils.h"

#define GetRendererContentSettingRules \
  GetRendererContentSettingRules_ChromiumImpl

// Brave's ContentSettingsType::BRAVE_COSMETIC_FILTERING,
// ContentSettingsType::BRAVE_SPEEDREADER,  and
// ContentSettingsType::BRAVE_COOKIES types use
// CONTENT_SETTING_DEFAULT as the initial default value, which is not a valid
// initial default value according to CanTrackLastVisit and
// ParseContentSettingValue: Note that |CONTENT_SETTING_DEFAULT| is encoded as a
// NULL value, so it is not allowed as an integer value. Also, see
// https://github.com/brave/brave-browser/issues/25733
#define BRAVE_CAN_TRACK_LAST_VISIT                             \
  if (type == ContentSettingsType::BRAVE_COOKIES ||            \
      type == ContentSettingsType::BRAVE_COSMETIC_FILTERING || \
      type == ContentSettingsType::BRAVE_SPEEDREADER) {        \
    return false;                                              \
  }

#include "src/components/content_settings/core/browser/content_settings_utils.cc"
#undef BRAVE_CAN_TRACK_LAST_VISIT
#undef GetRendererContentSettingRules

namespace content_settings {

void GetRendererContentSettingRules(const HostContentSettingsMap* map,
                                    RendererContentSettingRules* rules) {
  GetRendererContentSettingRules_ChromiumImpl(map, rules);
  std::pair<ContentSettingsType, ContentSettingsForOneType*> settings[] = {
      {ContentSettingsType::AUTOPLAY, &rules->autoplay_rules},
      {ContentSettingsType::BRAVE_FINGERPRINTING_V2,
       &rules->fingerprinting_rules},
      {ContentSettingsType::BRAVE_SHIELDS, &rules->brave_shields_rules},
      {ContentSettingsType::BRAVE_COSMETIC_FILTERING,
       &rules->cosmetic_filtering_rules},
  };
  for (const auto& setting : settings) {
    DCHECK(
        RendererContentSettingRules::IsRendererContentSetting(setting.first));
    *setting.second = map->GetSettingsForOneType(setting.first);
  }
}

}  // namespace content_settings
