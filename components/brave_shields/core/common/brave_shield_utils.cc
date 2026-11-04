// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/common/brave_shield_utils.h"

#include <map>
#include <optional>

#include "base/containers/map_util.h"
#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom-data-view.h"
#include "brave/components/webcompat/core/common/features.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
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

namespace {

// Site-specific BRAVE_FARBLING_LEVEL exceptions override the level derived
// from fingerprinting. The global default stays on the fingerprinting rules.
std::optional<ContentSetting> GetExplicitFarblingSetting(
    const ContentSettingsForOneType& rules,
    const GURL& primary_url) {
  for (const auto& rule : rules) {
    if (!rule.primary_pattern.Matches(primary_url)) {
      continue;
    }
    if (rule.primary_pattern.MatchesAllHosts()) {
      return std::nullopt;
    }
    return rule.GetContentSetting();
  }
  return std::nullopt;
}

content_settings::mojom::FarblingLevel FarblingLevelFromFingerprintingSetting(
    ContentSetting fp_setting) {
  if (fp_setting == CONTENT_SETTING_ALLOW) {
    return content_settings::mojom::FarblingLevel::OFF;
  }
  if (fp_setting == CONTENT_SETTING_BLOCK &&
      base::FeatureList::IsEnabled(
          features::kBraveShowStrictFingerprintingMode)) {
    return content_settings::mojom::FarblingLevel::MAXIMUM;
  }
  return content_settings::mojom::FarblingLevel::BALANCED;
}

}  // namespace

content_settings::mojom::FarblingLevel GetBraveFarblingLevelFromRules(
    const RendererContentSettingRules& rules,
    const GURL& primary_url,
    ContentSettingsType webcompat_settings_type) {
  if (!base::FeatureList::IsEnabled(features::kBraveFarbling) ||
      !primary_url.SchemeIsHTTPOrHTTPS()) {
    return content_settings::mojom::FarblingLevel::OFF;
  }

  ContentSetting shields_setting = CONTENT_SETTING_DEFAULT;
  for (const auto& rule : rules.brave_shields_rules) {
    if (rule.primary_pattern.Matches(primary_url) &&
        rule.secondary_pattern.Matches(GURL())) {
      shields_setting = rule.GetContentSetting();
      break;
    }
  }
  if (shields_setting == CONTENT_SETTING_BLOCK) {
    return content_settings::mojom::FarblingLevel::OFF;
  }

  content_settings::mojom::FarblingLevel level;
  if (const std::optional<ContentSetting> explicit_setting =
          GetExplicitFarblingSetting(rules.brave_farbling_level, primary_url)) {
    level = FarblingLevelFromContentSetting(*explicit_setting);
  } else {
    level = FarblingLevelFromFingerprintingSetting(
        GetBraveFPContentSettingFromRules(rules.fingerprinting_rules,
                                          primary_url));
  }
  if (level != content_settings::mojom::FarblingLevel::OFF &&
      webcompat_settings_type != ContentSettingsType::BRAVE_WEBCOMPAT_NONE &&
      GetBraveWebcompatContentSettingFromRules(
          rules.webcompat_rules, primary_url, webcompat_settings_type) ==
          CONTENT_SETTING_ALLOW) {
    return content_settings::mojom::FarblingLevel::OFF;
  }
  return level;
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

mojom::ContentSettingSource ConvertSettingsSource(
    const content_settings::SettingSource source) {
  switch (source) {
    case content_settings::SettingSource::kUser:
      return mojom::ContentSettingSource::kUser;
    case content_settings::SettingSource::kExtension:
      return mojom::ContentSettingSource::kExtension;
    case content_settings::SettingSource::kPolicy:
      return mojom::ContentSettingSource::kPolicy;
    case content_settings::SettingSource::kSupervised:
      return mojom::ContentSettingSource::kSupervised;
    case content_settings::SettingSource::kInstalledWebApp:
      return mojom::ContentSettingSource::kInstalledWebApp;
    case content_settings::SettingSource::kNone:
      return mojom::ContentSettingSource::kNone;
    case content_settings::SettingSource::kAllowList:
      return mojom::ContentSettingSource::kAllowList;
    case content_settings::SettingSource::kRemoteList:
      return mojom::ContentSettingSource::kRemoteList;
    case content_settings::SettingSource::kOsJavascriptOptimizer:
      return mojom::ContentSettingSource::kOsJavascriptOptimizer;
    case content_settings::SettingSource::kTest:
      return mojom::ContentSettingSource::kTest;
  }
  NOTREACHED();
}

}  // namespace brave_shields
