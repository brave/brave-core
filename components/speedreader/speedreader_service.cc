/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_service.h"

#include <string>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/common/speedreader_toolbar.mojom.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"

namespace speedreader {

namespace {

using mojom::ColumnWidth;
using mojom::FontFamily;
using mojom::FontSize;
using mojom::PlaybackSpeed;
using mojom::Theme;

}  // namespace

namespace features {

bool IsSpeedreaderEnabled() {
  return base::FeatureList::IsEnabled(kSpeedreaderFeature);
}

}  // namespace features

SpeedreaderService::SpeedreaderService(content::BrowserContext* browser_context,
                                       HostContentSettingsMap* content_rules)
    : browser_context_(browser_context),
      content_rules_(content_rules),
      prefs_(user_prefs::UserPrefs::Get(browser_context_)) {
  DCHECK(features::IsSpeedreaderEnabled());
}

SpeedreaderService::~SpeedreaderService() = default;

// static
void SpeedreaderService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  bool enabled_by_deault = false;

#if DCHECK_IS_ON()
  // Enable speedreader by default if the data collector command line key is
  // specified.
  constexpr const char kCollectSwitch[] = "speedreader-collect-test-data";
  enabled_by_deault =
      base::CommandLine::ForCurrentProcess()->HasSwitch(kCollectSwitch);
#endif

  registry->RegisterBooleanPref(kSpeedreaderPrefEnabled, enabled_by_deault);
  registry->RegisterBooleanPref(kSpeedreaderPrefEverEnabled, false);
  registry->RegisterListPref(kSpeedreaderPrefToggleCount);
  registry->RegisterIntegerPref(kSpeedreaderPrefPromptCount, 0);
  registry->RegisterIntegerPref(kSpeedreaderPrefTheme,
                                static_cast<int>(Theme::kNone));
  registry->RegisterIntegerPref(kSpeedreaderPrefFontSize,
                                static_cast<int>(FontSize::k100));
  registry->RegisterIntegerPref(kSpeedreaderPrefFontFamily,
                                static_cast<int>(FontFamily::kSans));
  registry->RegisterIntegerPref(kSpeedreaderPrefColumnWidth,
                                static_cast<int>(ColumnWidth::kNarrow));
  registry->RegisterStringPref(kSpeedreaderPrefTtsVoice, "");
  registry->RegisterIntegerPref(kSpeedreaderPrefTtsSpeed,
                                static_cast<int>(PlaybackSpeed::k100));
}

void SpeedreaderService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void SpeedreaderService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool SpeedreaderService::IsEnabledForAllSites() {
  return prefs_->GetBoolean(kSpeedreaderPrefEnabled);
}

ContentSetting SpeedreaderService::GetEnabledForSiteSetting(const GURL& url) {
  if (!url.is_valid()) {
    return CONTENT_SETTING_BLOCK;
  }
  if (!content_rules_) {
    return CONTENT_SETTING_BLOCK;
  }
  return content_rules_->GetContentSetting(
      url, GURL::EmptyGURL(), ContentSettingsType::BRAVE_SPEEDREADER);
}

ContentSetting SpeedreaderService::GetEnabledForSiteSetting(
    content::WebContents* contents) {
  if (!contents) {
    return CONTENT_SETTING_BLOCK;
  }
  return GetEnabledForSiteSetting(contents->GetLastCommittedURL());
}

bool SpeedreaderService::IsEnabledForSite(const GURL& url) {
  const auto setting = GetEnabledForSiteSetting(url);
  if (setting == CONTENT_SETTING_BLOCK) {
    return false;
  } else if (setting == CONTENT_SETTING_ALLOW) {
    return true;
  }
  return IsEnabledForAllSites();
}

bool SpeedreaderService::IsEnabledForSite(content::WebContents* contents) {
  if (!contents) {
    return false;
  }
  return IsEnabledForSite(contents->GetLastCommittedURL());
}

void SpeedreaderService::EnableForAllSites(bool enabled) {
  if (IsEnabledForAllSites() == enabled) {
    return;
  }
  prefs_->SetBoolean(kSpeedreaderPrefEnabled, enabled);

  for (auto& o : observers_) {
    o.OnAllSitesEnableSettingChanged(enabled);
  }
}

void SpeedreaderService::EnableForSite(const GURL& url, bool enabled) {
  if (!url.is_valid()) {
    return;
  }
  const ContentSetting setting =
      enabled ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;
  if (GetEnabledForSiteSetting(url) == setting) {
    return;
  }
  if (!content_rules_) {
    return;
  }
  // Rule covers all protocols and pages.
  const auto pattern =
      ContentSettingsPattern::FromString("*://" + url.host() + "/*");
  if (!pattern.IsValid()) {
    return;
  }

  content_rules_->SetContentSettingCustomScope(
      pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_SPEEDREADER, setting);
}

void SpeedreaderService::EnableForSite(content::WebContents* contents,
                                       bool enabled) {
  if (contents) {
    EnableForSite(contents->GetLastCommittedURL(), enabled);
    for (auto& o : observers_) {
      o.OnSiteEnableSettingChanged(contents, enabled);
    }
  }
}

void SpeedreaderService::SetAppearanceSettings(
    const mojom::AppearanceSettings& appearance_settings) {
  prefs_->SetInteger(kSpeedreaderPrefTheme,
                     static_cast<int>(appearance_settings.theme));
  prefs_->SetInteger(kSpeedreaderPrefFontSize,
                     static_cast<int>(appearance_settings.fontSize));
  prefs_->SetInteger(kSpeedreaderPrefFontFamily,
                     static_cast<int>(appearance_settings.fontFamily));
  prefs_->SetInteger(kSpeedreaderPrefColumnWidth,
                     static_cast<int>(appearance_settings.columnWidth));

  for (auto& o : observers_) {
    o.OnAppearanceSettingsChanged(appearance_settings);
  }
}

mojom::AppearanceSettings SpeedreaderService::GetAppearanceSettings() const {
  mojom::AppearanceSettings appearance_settings;

  appearance_settings.theme =
      static_cast<mojom::Theme>(prefs_->GetInteger(kSpeedreaderPrefTheme));
  appearance_settings.fontSize =
      static_cast<FontSize>(prefs_->GetInteger(kSpeedreaderPrefFontSize));
  appearance_settings.fontFamily =
      static_cast<FontFamily>(prefs_->GetInteger(kSpeedreaderPrefFontFamily));
  appearance_settings.columnWidth =
      static_cast<ColumnWidth>(prefs_->GetInteger(kSpeedreaderPrefColumnWidth));

  return appearance_settings;
}

void SpeedreaderService::SetTtsSettings(
    const mojom::TtsSettings& tts_settings) {
  prefs_->SetString(kSpeedreaderPrefTtsVoice, tts_settings.voice);
  prefs_->SetInteger(kSpeedreaderPrefTtsSpeed,
                     static_cast<int>(tts_settings.speed));
  for (auto& o : observers_) {
    o.OnTtsSettingsChanged(tts_settings);
  }
}

mojom::TtsSettings SpeedreaderService::GetTtsSettings() const {
  mojom::TtsSettings settings;

  settings.voice = prefs_->GetString(kSpeedreaderPrefTtsVoice);
  settings.speed =
      static_cast<PlaybackSpeed>(prefs_->GetInteger(kSpeedreaderPrefTtsSpeed));

  return settings;
}

std::string SpeedreaderService::GetThemeName() const {
  const auto settings = GetAppearanceSettings();
  switch (settings.theme) {
    default:
      return {};
    case Theme::kNone:
      return {};
    case Theme::kLight:
      return "light";
    case Theme::kSepia:
      return "sepia";
    case Theme::kDark:
      return "dark";
  }
}

std::string SpeedreaderService::GetFontSizeName() const {
  return base::NumberToString(
      static_cast<int>(GetAppearanceSettings().fontSize));
}

std::string SpeedreaderService::GetFontFamilyName() const {
  switch (GetAppearanceSettings().fontFamily) {
    case FontFamily::kSans:
      return "sans";
    case FontFamily::kSerif:
      return "serif";
    case FontFamily::kMono:
      return "mono";
    case FontFamily::kDyslexic:
      return "dyslexic";
  }
}

std::string SpeedreaderService::GetColumnWidth() const {
  switch (GetAppearanceSettings().columnWidth) {
    case ColumnWidth::kNarrow:
      return "narrow";
    case ColumnWidth::kWide:
      return "wide";
  }
  return "narrow";
}

}  // namespace speedreader
