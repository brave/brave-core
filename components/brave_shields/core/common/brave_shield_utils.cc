// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/common/brave_shield_utils.h"

#include <map>

#include "base/containers/map_util.h"
#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/webcompat/core/common/features.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace brave_shields {

ContentSetting GetBraveFPContentSettingFromRules(
    const ContentSettingsForOneType& fp_rules,
    const GURL& primary_url) {
  static const base::NoDestructor<ContentSettingsPattern> kBalancedRule(
      ContentSettingsPattern::FromString("https://balanced"));
  ContentSettingPatternSource fp_rule;
  for (const auto& rule : fp_rules) {
    if (rule.secondary_pattern == *kBalancedRule) {
      continue;
    }
    if (rule.primary_pattern.Matches(primary_url)) {
      return rule.GetContentSetting();
    }
  }

  return CONTENT_SETTING_DEFAULT;
}

ContentSetting GetBraveWebcompatContentSettingFromRules(
    const std::map<ContentSettingsType, ContentSettingsForOneType>&
        webcompat_rules,
    const GURL& primary_url,
    const ContentSettingsType content_settings_type) {
  if (!base::FeatureList::IsEnabled(
          webcompat::features::kBraveWebcompatExceptionsService)) {
    return CONTENT_SETTING_DEFAULT;
  }
  const auto* rules = base::FindOrNull(webcompat_rules, content_settings_type);
  if (!rules) {
    return CONTENT_SETTING_DEFAULT;
  }
  for (const auto& rule : *rules) {
    if (rule.primary_pattern.Matches(primary_url)) {
      return rule.GetContentSetting();
    }
  }
  return CONTENT_SETTING_DEFAULT;
}

ShieldsSettingCounts GetSettingCountFromRules(
    const ContentSettingsForOneType& fp_rules) {
  ShieldsSettingCounts result = {};

  for (const auto& rule : fp_rules) {
    if (rule.primary_pattern.MatchesAllHosts()) {
      continue;
    }
    if (rule.GetContentSetting() == CONTENT_SETTING_ALLOW) {
      result.allow++;
    } else if (rule.GetContentSetting() == CONTENT_SETTING_BLOCK) {
      result.aggressive++;
    } else {
      result.standard++;
    }
  }

  return result;
}

ShieldsSettingCounts GetSettingCountFromCosmeticFilteringRules(
    const ContentSettingsForOneType& fp_rules) {
  ShieldsSettingCounts result = {};

  for (const auto& rule : fp_rules) {
    if (rule.primary_pattern.MatchesAllHosts()) {
      continue;
    }
    switch (CosmeticFilteringSetting::FromValue(rule.setting_value)) {
      case ControlType::ALLOW:
        ++result.allow;
        break;
      case ControlType::BLOCK:
        ++result.aggressive;
        break;
      default:
        ++result.standard;
    }
  }

  return result;
}

bool IsAdblockOnlyModeFeatureEnabled() {
  return base::FeatureList::IsEnabled(features::kAdblockOnlyMode);
}

bool IsBraveShieldsAdBlockOnlyModeEnabled(PrefService* local_state) {
  CHECK(local_state);
  return local_state->GetBoolean(prefs::kAdBlockOnlyModeEnabled);
}

void SetBraveShieldsAdBlockOnlyModeEnabled(PrefService* local_state,
                                           bool enabled) {
  CHECK(local_state);
  local_state->SetBoolean(prefs::kAdBlockOnlyModeEnabled, enabled);
}

std::string GetLanguageCodeFromLocale(const std::string& locale) {
  const std::string::size_type loc = locale.find("-");
  if (loc == std::string::npos) {
    return base::ToLowerASCII(locale);
  }
  return base::ToLowerASCII(locale.substr(0, loc));
}

bool IsAdblockOnlyModeSupportedForLocale(const std::string& locale) {
  return kAdblockOnlyModeSupportedLanguageCodes.contains(
      GetLanguageCodeFromLocale(locale));
}

void ManageAdBlockOnlyModeByLocale(PrefService* local_state,
                                   const std::string& locale) {
  if (!base::FeatureList::IsEnabled(
          brave_shields::features::kAdblockOnlyMode)) {
    return;
  }

  if (!IsAdblockOnlyModeSupportedForLocale(locale)) {
    // If the current locale is not supported and Ad Block Only mode is enabled,
    // then disable Ad Block Only mode and set
    // `brave.shields.adblock_only_mode_was_enabled_for_supported_locale` pref
    // value to true.
    if (local_state->GetBoolean(prefs::kAdBlockOnlyModeEnabled)) {
      local_state->SetBoolean(prefs::kAdBlockOnlyModeEnabled, false);
      local_state->SetBoolean(
          prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale, true);
    }
  } else {
    // If the current locale is supported, Ad Block Only mode is not enabled and
    // was enabled previously for a supported locale, enable it.
    if (!local_state->GetBoolean(prefs::kAdBlockOnlyModeEnabled) &&
        local_state->GetBoolean(
            prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale)) {
      local_state->SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
      local_state->SetBoolean(
          prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale, false);
    }
  }
}

}  // namespace brave_shields
