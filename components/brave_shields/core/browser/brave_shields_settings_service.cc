// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"

#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace brave_shields {

namespace {

mojom::ContentSettingsOverrideSource ConvertSettingsSource(
    const content_settings::SettingSource source) {
  switch (source) {
    case content_settings::SettingSource::kUser:
      return mojom::ContentSettingsOverrideSource::kUser;
    case content_settings::SettingSource::kExtension:
      return mojom::ContentSettingsOverrideSource::kExtension;
    case content_settings::SettingSource::kPolicy:
      return mojom::ContentSettingsOverrideSource::kPolicy;
    case content_settings::SettingSource::kSupervised:
      return mojom::ContentSettingsOverrideSource::kSupervised;
    case content_settings::SettingSource::kInstalledWebApp:
      return mojom::ContentSettingsOverrideSource::kInstalledWebApp;
    case content_settings::SettingSource::kNone:
      return mojom::ContentSettingsOverrideSource::kNone;
    case content_settings::SettingSource::kAllowList:
      return mojom::ContentSettingsOverrideSource::kAllowList;
    case content_settings::SettingSource::kTpcdGrant:
      return mojom::ContentSettingsOverrideSource::kTpcdGrant;
    case content_settings::SettingSource::kRemoteList:
      return mojom::ContentSettingsOverrideSource::kRemoteList;
    case content_settings::SettingSource::kOsJavascriptOptimizer:
      return mojom::ContentSettingsOverrideSource::kOsJavascriptOptimizer;
    case content_settings::SettingSource::kTest:
      return mojom::ContentSettingsOverrideSource::kTest;
  }
  NOTREACHED();
}

}  // namespace

BraveShieldsSettingsService::BraveShieldsSettingsService(
    HostContentSettingsMap& host_content_settings_map,
    PrefService* local_state,
    PrefService* profile_prefs)
    : host_content_settings_map_(host_content_settings_map),
      local_state_(local_state),
      profile_prefs_(profile_prefs) {}

BraveShieldsSettingsService::~BraveShieldsSettingsService() = default;

void BraveShieldsSettingsService::SetBraveShieldsEnabled(bool is_enabled,
                                                         const GURL& url) {
  brave_shields::SetBraveShieldsEnabled(&*host_content_settings_map_,
                                        is_enabled, url, local_state_);
}

bool BraveShieldsSettingsService::GetBraveShieldsEnabled(const GURL& url) {
  return brave_shields::GetBraveShieldsEnabled(&*host_content_settings_map_,
                                               url);
}

void BraveShieldsSettingsService::SetDefaultAdBlockMode(
    mojom::AdBlockMode mode) {
  SetAdBlockMode(mode, GURL());
}

mojom::AdBlockMode BraveShieldsSettingsService::GetDefaultAdBlockMode() {
  return GetAdBlockMode(GURL());
}

void BraveShieldsSettingsService::SetAdBlockMode(mojom::AdBlockMode mode,
                                                 const GURL& url) {
  ControlType control_type_ad;
  ControlType control_type_cosmetic;

  if (mode == mojom::AdBlockMode::ALLOW) {
    control_type_ad = ControlType::ALLOW;
  } else {
    control_type_ad = ControlType::BLOCK;
  }

  if (mode == mojom::AdBlockMode::AGGRESSIVE) {
    control_type_cosmetic = ControlType::BLOCK;  // aggressive
  } else if (mode == mojom::AdBlockMode::STANDARD) {
    control_type_cosmetic = ControlType::BLOCK_THIRD_PARTY;  // standard
  } else {
    control_type_cosmetic = ControlType::ALLOW;  // allow
  }

  brave_shields::SetAdControlType(&*host_content_settings_map_, control_type_ad,
                                  url, local_state_);

  brave_shields::SetCosmeticFilteringControlType(&*host_content_settings_map_,
                                                 control_type_cosmetic, url,
                                                 local_state_, profile_prefs_);
}

mojom::AdBlockMode BraveShieldsSettingsService::GetAdBlockMode(
    const GURL& url) {
  ControlType control_type_ad =
      brave_shields::GetAdControlType(&*host_content_settings_map_, url);

  ControlType control_type_cosmetic =
      brave_shields::GetCosmeticFilteringControlType(
          &*host_content_settings_map_, url);

  if (control_type_ad == ControlType::ALLOW) {
    return mojom::AdBlockMode::ALLOW;
  }

  if (control_type_cosmetic == ControlType::BLOCK) {
    return mojom::AdBlockMode::AGGRESSIVE;
  } else {
    return mojom::AdBlockMode::STANDARD;
  }
}

void BraveShieldsSettingsService::SetDefaultFingerprintMode(
    mojom::FingerprintMode mode) {
  SetFingerprintMode(mode, GURL());
}

mojom::FingerprintMode
BraveShieldsSettingsService::GetDefaultFingerprintMode() {
  return GetFingerprintMode(GURL());
}

void BraveShieldsSettingsService::SetFingerprintMode(
    mojom::FingerprintMode mode,
    const GURL& url) {
#if BUILDFLAG(IS_IOS)
  /// Strict FingerprintMode is not supported on iOS
  CHECK(mode != mojom::FingerprintMode::STRICT_MODE);
#endif

  ControlType control_type;

  if (mode == mojom::FingerprintMode::ALLOW_MODE) {
    control_type = ControlType::ALLOW;
  } else if (mode == mojom::FingerprintMode::STRICT_MODE) {
    control_type = ControlType::BLOCK;
  } else {
    control_type = ControlType::DEFAULT;  // STANDARD_MODE
  }

  brave_shields::SetFingerprintingControlType(&*host_content_settings_map_,
                                              control_type, url, local_state_,
                                              profile_prefs_);
}

mojom::FingerprintMode BraveShieldsSettingsService::GetFingerprintMode(
    const GURL& url) {
  ControlType control_type = brave_shields::GetFingerprintingControlType(
      &*host_content_settings_map_, url);

  if (control_type == ControlType::ALLOW) {
    return mojom::FingerprintMode::ALLOW_MODE;
  } else if (control_type == ControlType::BLOCK) {
#if BUILDFLAG(IS_IOS)
    /// Strict FingerprintMode is not supported on iOS.
    /// In case of sync'd setting, return standard mode.
    return mojom::FingerprintMode::STANDARD_MODE;
#else
    return mojom::FingerprintMode::STRICT_MODE;
#endif
  } else {
    return mojom::FingerprintMode::STANDARD_MODE;
  }
}

void BraveShieldsSettingsService::SetNoScriptEnabledByDefault(bool is_enabled) {
  SetNoScriptEnabled(is_enabled, GURL());
}

bool BraveShieldsSettingsService::IsNoScriptEnabledByDefault() {
  return IsNoScriptEnabled(GURL());
}

void BraveShieldsSettingsService::SetNoScriptEnabled(bool is_enabled,
                                                     const GURL& url) {
  ControlType control_type =
      is_enabled ? ControlType::BLOCK : ControlType::ALLOW;
  brave_shields::SetNoScriptControlType(&*host_content_settings_map_,
                                        control_type, url, local_state_);
}

bool BraveShieldsSettingsService::IsNoScriptEnabled(const GURL& url) {
  ControlType control_type =
      brave_shields::GetNoScriptControlType(&*host_content_settings_map_, url);

  return control_type != ControlType::ALLOW;
}

void BraveShieldsSettingsService::SetDefaultAutoShredMode(
    mojom::AutoShredMode mode) {
  SetAutoShredMode(mode, GURL());
}

mojom::AutoShredMode BraveShieldsSettingsService::GetDefaultAutoShredMode() {
  return GetAutoShredMode(GURL());
}

void BraveShieldsSettingsService::SetAutoShredMode(mojom::AutoShredMode mode,
                                                   const GURL& url) {
  // Shred and AutoShred delete data at the eTLD+1 boundary, because that’s
  // the Web’s cookie boundary, so we must use the domain pattern to align
  // with how browsers enforce storage boundaries.
  auto primary_pattern = content_settings::CreateDomainPattern(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  host_content_settings_map_->SetWebsiteSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      AutoShredSetting::kContentSettingsType, AutoShredSetting::ToValue(mode));
}

mojom::AutoShredMode BraveShieldsSettingsService::GetAutoShredMode(
    const GURL& url) {
  return AutoShredSetting::FromValue(
      host_content_settings_map_->GetWebsiteSetting(
          url, GURL(), AutoShredSetting::kContentSettingsType));
}

void BraveShieldsSettingsService::SetDefaultAutoShredMode(
    mojom::AutoShredMode mode) {
  SetAutoShredMode(mode, GURL());
}

mojom::AutoShredMode BraveShieldsSettingsService::GetDefaultAutoShredMode() {
  return GetAutoShredMode(GURL());
}

void BraveShieldsSettingsService::SetAutoShredMode(mojom::AutoShredMode mode,
                                                   const GURL& url) {
  // Shred and AutoShred delete data at the eTLD+1 boundary, because that’s
  // the Web’s cookie boundary, so we must use the domain pattern to align
  // with how browsers enforce storage boundaries.
  auto primary_pattern = content_settings::CreateDomainPattern(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  host_content_settings_map_->SetWebsiteSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      AutoShredSetting::kContentSettingsType, AutoShredSetting::ToValue(mode));
}

mojom::AutoShredMode BraveShieldsSettingsService::GetAutoShredMode(
    const GURL& url) {
  return AutoShredSetting::FromValue(
      host_content_settings_map_->GetWebsiteSetting(
          url, GURL(), AutoShredSetting::kContentSettingsType));
}

mojom::ContentSettingsOverriddenDataPtr
BraveShieldsSettingsService::GetJsContentSettingsOverriddenData(
    const GURL& url) {
  content_settings::SettingInfo info;
  const auto rule = host_content_settings_map_->GetContentSetting(
      url, GURL(), content_settings::mojom::ContentSettingsType::JAVASCRIPT,
      &info);

  if (info.source == content_settings::SettingSource::kUser) {
    return mojom::ContentSettingsOverriddenData::New(
        mojom::ContentSettingsOverriddenStatus::kNotSet,
        ConvertSettingsSource(info.source));
  }

  return mojom::ContentSettingsOverriddenData::New(
      rule == CONTENT_SETTING_BLOCK
          ? mojom::ContentSettingsOverriddenStatus::kBlocked
          : mojom::ContentSettingsOverriddenStatus::kAllowed,
      ConvertSettingsSource(info.source));
}

}  // namespace brave_shields
