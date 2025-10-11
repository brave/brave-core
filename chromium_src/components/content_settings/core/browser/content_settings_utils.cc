/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/browser/content_settings_utils.h"

#include <iostream>

#include "base/check.h"

#define GetRendererContentSettingRules \
  GetRendererContentSettingRules_ChromiumImpl

#define GetTypesWithTemporaryGrants GetTypesWithTemporaryGrants_ChromiumImpl
#define GetTypesWithTemporaryGrantsInHcsm \
  GetTypesWithTemporaryGrantsInHcsm_ChromiumImpl

#include <components/content_settings/core/browser/content_settings_utils.cc>
#undef GetTypesWithTemporaryGrantsInHcsm
#undef GetTypesWithTemporaryGrants
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
      {ContentSettingsType::JAVASCRIPT,
       &rules->extension_created_java_script_rules}};
  for (const auto& setting : settings) {
    DCHECK(
        RendererContentSettingRules::IsRendererContentSetting(setting.first));
    *setting.second = map->GetSettingsForOneType(setting.first);
  }
  for (auto webcompat_settings_type = ContentSettingsType::BRAVE_WEBCOMPAT_NONE;
       webcompat_settings_type != ContentSettingsType::BRAVE_WEBCOMPAT_ALL;
       webcompat_settings_type = static_cast<ContentSettingsType>(
           static_cast<int32_t>(webcompat_settings_type) + 1)) {
    rules->webcompat_rules[webcompat_settings_type] =
        map->GetSettingsForOneType(webcompat_settings_type);
  }
}

const std::vector<ContentSettingsType>& GetTypesWithTemporaryGrants() {
  static base::NoDestructor<const std::vector<ContentSettingsType>> types;
  return *types;
}

const std::vector<ContentSettingsType>& GetTypesWithTemporaryGrantsInHcsm() {
  static base::NoDestructor<const std::vector<ContentSettingsType>> types;
  return *types;
}

}  // namespace content_settings
