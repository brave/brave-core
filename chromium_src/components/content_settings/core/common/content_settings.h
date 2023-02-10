/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_H_

#define RendererContentSettingRules RendererContentSettingRules_ChromiumImpl

#include "src/components/content_settings/core/common/content_settings.h"  // IWYU pragma: export

#undef RendererContentSettingRules

struct RendererContentSettingRules
    : public RendererContentSettingRules_ChromiumImpl {
  RendererContentSettingRules();
  ~RendererContentSettingRules();
  RendererContentSettingRules(const RendererContentSettingRules& rules);
  RendererContentSettingRules(RendererContentSettingRules&& rules);
  RendererContentSettingRules& operator=(
      const RendererContentSettingRules& rules);
  RendererContentSettingRules& operator=(RendererContentSettingRules&& rules);

  static bool IsRendererContentSetting(ContentSettingsType content_type);

  void FilterRulesByOutermostMainFrameURL(const GURL& outermost_main_frame_url);

  ContentSettingsForOneType autoplay_rules;
  ContentSettingsForOneType fingerprinting_rules;
  ContentSettingsForOneType brave_shields_rules;
  ContentSettingsForOneType cosmetic_filtering_rules;
};

namespace content_settings {

bool IsExplicitSetting(const ContentSettingPatternSource& setting);
bool IsExplicitSetting(const SettingInfo& setting);

}  // namespace content_settings

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_H_
