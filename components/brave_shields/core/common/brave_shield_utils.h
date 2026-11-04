// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_UTILS_H_

#include <map>

#include "brave/components/brave_shields/core/common/shields_settings.mojom-data-view.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings.mojom.h"

class GURL;

namespace brave_shields {

struct ShieldsSettingCounts {
  int allow;
  int standard;
  int aggressive;
};

ContentSetting GetBraveFPContentSettingFromRules(
    const ContentSettingsForOneType& fp_rules,
    const GURL& primary_url);

ContentSetting GetBraveWebcompatContentSettingFromRules(
    const std::map<ContentSettingsType, ContentSettingsForOneType>&
        webcompat_rules,
    const GURL& primary_url,
    const ContentSettingsType content_settings_type);

// ALLOW is farbling off, BLOCK is maximum, and ASK/DEFAULT are balanced.
inline content_settings::mojom::FarblingLevel FarblingLevelFromContentSetting(
    ContentSetting setting) {
  switch (setting) {
    case CONTENT_SETTING_ALLOW:
      return content_settings::mojom::FarblingLevel::OFF;
    case CONTENT_SETTING_BLOCK:
      return content_settings::mojom::FarblingLevel::MAXIMUM;
    case CONTENT_SETTING_ASK:
    case CONTENT_SETTING_DEFAULT:
      return content_settings::mojom::FarblingLevel::BALANCED;
    default:
      return content_settings::mojom::FarblingLevel::BALANCED;
  }
}

// Resolves BRAVE_FARBLING_LEVEL from renderer content setting rules. Shields
// down and a webcompat exception both force farbling off.
content_settings::mojom::FarblingLevel GetBraveFarblingLevelFromRules(
    const RendererContentSettingRules& rules,
    const GURL& primary_url,
    ContentSettingsType webcompat_settings_type);

ShieldsSettingCounts GetSettingCountFromRules(
    const ContentSettingsForOneType& fp_rules);

ShieldsSettingCounts GetSettingCountFromCosmeticFilteringRules(
    const ContentSettingsForOneType& fp_rules);

mojom::ContentSettingSource ConvertSettingsSource(
    const content_settings::SettingSource source);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELD_UTILS_H_
