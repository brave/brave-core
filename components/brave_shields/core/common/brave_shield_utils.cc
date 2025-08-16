// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/common/brave_shield_utils.h"

#include <map>
#include <set>
#include <string>

#include "base/containers/map_util.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
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

ShieldsSettingCounts GetFPSettingCountFromRules(
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

ShieldsSettingCounts GetAdsSettingCountFromRules(
    const ContentSettingsForOneType& ads_rules) {
  ShieldsSettingCounts result = {};

  std::set<std::string> block_set;
  // Look at primary rules
  for (const auto& rule : ads_rules) {
    if (rule.primary_pattern.MatchesAllHosts() ||
        !rule.secondary_pattern.MatchesAllHosts()) {
      continue;
    }
    if (rule.GetContentSetting() == CONTENT_SETTING_ALLOW) {
      result.allow++;
    } else {
      block_set.insert(rule.primary_pattern.ToString());
    }
  }

  // And then look at "first party" rules
  for (const auto& rule : ads_rules) {
    if (rule.primary_pattern.MatchesAllHosts() ||
        rule.secondary_pattern.MatchesAllHosts() ||
        !block_set.contains(rule.primary_pattern.ToString())) {
      continue;
    }
    if (rule.GetContentSetting() == CONTENT_SETTING_BLOCK) {
      result.aggressive++;
    } else {
      result.standard++;
    }
  }

  return result;
}

bool IsAdblockOnlyModeFeatureEnabled() {
  return base::FeatureList::IsEnabled(features::kAdblockOnlyMode);
}

bool IsAdblockOnlyModeSupportedForLocale(const std::string& locale) {
  return kAdblockOnlyModeSupportedLanguageCodes.contains(
      GetLanguageCodeFromLocale(locale));
}

bool GetBraveShieldsAdBlockOnlyModeEnabled(PrefService* prefs) {
  return prefs &&
         prefs->FindPreference(prefs::kAdblockAdBlockOnlyModeEnabled) &&
         prefs->GetBoolean(prefs::kAdblockAdBlockOnlyModeEnabled);
}

void SetBraveShieldsAdBlockOnlyModeEnabled(PrefService* prefs, bool enabled) {
  if (prefs) {
    prefs->SetBoolean(prefs::kAdblockAdBlockOnlyModeEnabled, enabled);
  }
}

std::string GetLanguageCodeFromLocale(const std::string& locale) {
  const std::string::size_type loc = locale.find("-");
  if (loc == std::string::npos) {
    return base::ToLowerASCII(locale);
  }
  return base::ToLowerASCII(locale.substr(0, loc));
}

}  // namespace brave_shields
