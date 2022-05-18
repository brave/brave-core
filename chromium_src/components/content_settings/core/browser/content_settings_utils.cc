/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/browser/content_settings_utils.h"

#include "brave/components/brave_shields/common/brave_shield_constants.h"

#define GetRendererContentSettingRules \
  GetRendererContentSettingRules_ChromiumImpl

#include "src/components/content_settings/core/browser/content_settings_utils.cc"
#undef GetRendererContentSettingRules

namespace content_settings {

void GetRendererContentSettingRules(const HostContentSettingsMap* map,
                                    RendererContentSettingRules* rules) {
  GetRendererContentSettingRules_ChromiumImpl(map, rules);
  map->GetSettingsForOneType(ContentSettingsType::AUTOPLAY,
                             &(rules->autoplay_rules));
  map->GetSettingsForOneType(ContentSettingsType::BRAVE_FINGERPRINTING_V2,
                             &(rules->fingerprinting_rules));
  map->GetSettingsForOneType(ContentSettingsType::BRAVE_SHIELDS,
                             &(rules->brave_shields_rules));
  map->GetSettingsForOneType(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                             &(rules->cosmetic_filtering_rules));
}

}  // namespace content_settings
