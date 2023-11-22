/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/common/content_settings.h"

#include <vector>

#define RendererContentSettingRules RendererContentSettingRules_ChromiumImpl

#include "src/components/content_settings/core/common/content_settings.cc"

#undef RendererContentSettingRules

RendererContentSettingRules::RendererContentSettingRules() = default;
RendererContentSettingRules::~RendererContentSettingRules() = default;

RendererContentSettingRules::RendererContentSettingRules(
    const RendererContentSettingRules&) = default;

RendererContentSettingRules::RendererContentSettingRules(
    RendererContentSettingRules&& rules) = default;

RendererContentSettingRules& RendererContentSettingRules::operator=(
    const RendererContentSettingRules& rules) = default;

RendererContentSettingRules& RendererContentSettingRules::operator=(
    RendererContentSettingRules&& rules) = default;

// static
bool RendererContentSettingRules::IsRendererContentSetting(
    ContentSettingsType content_type) {
  return RendererContentSettingRules_ChromiumImpl::IsRendererContentSetting(
             content_type) ||
         content_type == ContentSettingsType::AUTOPLAY ||
         content_type == ContentSettingsType::BRAVE_COSMETIC_FILTERING ||
         content_type == ContentSettingsType::BRAVE_FINGERPRINTING_V2 ||
         content_type == ContentSettingsType::BRAVE_GOOGLE_SIGN_IN ||
         content_type == ContentSettingsType::BRAVE_LOCALHOST_ACCESS ||
         content_type == ContentSettingsType::BRAVE_SHIELDS;
}

void RendererContentSettingRules::FilterRulesByOutermostMainFrameURL(
    const GURL& outermost_main_frame_url) {
  RendererContentSettingRules_ChromiumImpl::FilterRulesByOutermostMainFrameURL(
      outermost_main_frame_url);
  FilterRulesForType(autoplay_rules, outermost_main_frame_url);
  FilterRulesForType(brave_shields_rules, outermost_main_frame_url);
  // FilterRulesForType has a DCHECK on the size and these fail (for now)
  // because they incorrectly use CONTENT_SETTINGS_DEFAULT as a distinct setting
  std::erase_if(
      cosmetic_filtering_rules,
      [&outermost_main_frame_url](const ContentSettingPatternSource& source) {
        return !source.primary_pattern.Matches(outermost_main_frame_url);
      });
  std::erase_if(
      fingerprinting_rules,
      [&outermost_main_frame_url](const ContentSettingPatternSource& source) {
        return !source.primary_pattern.Matches(outermost_main_frame_url);
      });
}

namespace content_settings {
namespace {

bool IsExplicitSetting(const ContentSettingsPattern& primary_pattern,
                       const ContentSettingsPattern& secondary_pattern) {
  return !primary_pattern.MatchesAllHosts() ||
         !secondary_pattern.MatchesAllHosts();
}

}  // namespace

bool IsExplicitSetting(const ContentSettingPatternSource& setting) {
  return IsExplicitSetting(setting.primary_pattern, setting.secondary_pattern);
}

bool IsExplicitSetting(const SettingInfo& setting) {
  return IsExplicitSetting(setting.primary_pattern, setting.secondary_pattern);
}

}  // namespace content_settings
