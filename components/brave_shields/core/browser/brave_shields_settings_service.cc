// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"

#include "base/hash/hash.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace brave_shields {

// Used for stable farbling token generation in tests when is set to non-zero.
// Non-anonymous to be accesible from ":test_support" target.
uint32_t g_stable_farbling_tokens_seed = 0;

namespace {

base::DictValue GetShieldsMetadata(HostContentSettingsMap* map,
                                   const GURL& url) {
  auto shields_metadata_value = map->GetWebsiteSetting(
      url, url, ContentSettingsType::BRAVE_SHIELDS_METADATA);
  if (auto* shields_metadata_dict = shields_metadata_value.GetIfDict()) {
    return std::move(*shields_metadata_dict);
  }
  return base::DictValue();
}

void SetShieldsMetadata(HostContentSettingsMap* map,
                        const GURL& url,
                        base::DictValue shields_metadata) {
  map->SetWebsiteSettingDefaultScope(
      url, url, ContentSettingsType::BRAVE_SHIELDS_METADATA,
      base::Value(std::move(shields_metadata)));
}

// Returns a 64-bit persistent hash of |data| (two rounds of PersistentHash).
uint64_t PersistentHashU64(base::span<const uint8_t> data) {
  const uint32_t hash = base::PersistentHash(data);
  return (static_cast<uint64_t>(hash) << 32) |
         base::PersistentHash(base::byte_span_from_ref(hash));
}

base::Token CreateStableFarblingToken(const GURL& url) {
  const uint32_t high =
      base::PersistentHash(url.host()) + g_stable_farbling_tokens_seed - 1;
  const uint32_t low = base::PersistentHash(base::byte_span_from_ref(high));
  return base::Token(high, low);
}

}  // namespace

BraveShieldsSettingsService::BraveShieldsSettingsService(
    HostContentSettingsMap& host_content_settings_map,
    PrefService* local_state,
    PrefService* profile_prefs)
    : host_content_settings_map_(host_content_settings_map),
      local_state_(local_state),
      profile_prefs_(profile_prefs) {
  CHECK(profile_prefs_);
}

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

#if !BUILDFLAG(IS_IOS)
bool BraveShieldsSettingsService::GetForgetFirstPartyStorageEnabled(
    const GURL& url) {
  ContentSetting setting = host_content_settings_map_->GetContentSetting(
      url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE);

  return setting == CONTENT_SETTING_BLOCK;
}

void BraveShieldsSettingsService::SetForgetFirstPartyStorageEnabled(
    bool is_enabled,
    const GURL& url) {
  auto primary_pattern = content_settings::CreateDomainPattern(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  host_content_settings_map_->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
      is_enabled ? CONTENT_SETTING_BLOCK : CONTENT_SETTING_ALLOW);

  MaybeRecordShieldsUsageP3A(kChangedPerSiteShields, local_state_);
  RecordForgetFirstPartySetting(&*host_content_settings_map_);
}
#endif

void BraveShieldsSettingsService::SetDefaultAutoShredMode(
    mojom::AutoShredMode mode) {
  SetAutoShredMode(mode, GURL());
}

mojom::AutoShredMode BraveShieldsSettingsService::GetDefaultAutoShredMode() {
  return GetAutoShredMode(GURL());
}

void BraveShieldsSettingsService::SetAutoShredMode(mojom::AutoShredMode mode,
                                                   const GURL& url) {
  CHECK(base::FeatureList::IsEnabled(
      brave_shields::features::kBraveShredFeature));

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

  ReportAutoShredSettingsP3A(*host_content_settings_map_);
}

mojom::AutoShredMode BraveShieldsSettingsService::GetAutoShredMode(
    const GURL& url) {
  CHECK(base::FeatureList::IsEnabled(
      brave_shields::features::kBraveShredFeature));
  return AutoShredSetting::FromValue(
      host_content_settings_map_->GetWebsiteSetting(
          url, GURL(), AutoShredSetting::kContentSettingsType));
}

bool BraveShieldsSettingsService::IsJsBlockingEnforced(const GURL& url) {
  const auto js_content_settings_overridden_data =
      GetJsContentSettingOverriddenData(url);
  return js_content_settings_overridden_data &&
         js_content_settings_overridden_data->status ==
             ::ContentSetting::CONTENT_SETTING_BLOCK;
}

mojom::ContentSettingsOverriddenDataPtr
BraveShieldsSettingsService::GetJsContentSettingOverriddenData(
    const GURL& url) {
  content_settings::SettingInfo info;
  const auto rule = host_content_settings_map_->GetContentSetting(
      url, GURL(), content_settings::mojom::ContentSettingsType::JAVASCRIPT,
      &info);

  // No override
  if (info.source == content_settings::SettingSource::kUser) {
    return nullptr;
  }

  return mojom::ContentSettingsOverriddenData::New(
      rule, ConvertSettingsSource(info.source));
}

bool BraveShieldsSettingsService::IsShieldsDisabledOnAnyHostMatchingDomainOf(
    const GURL& url) const {
  // First check the exact domain
  if (CONTENT_SETTING_BLOCK ==
      host_content_settings_map_->GetContentSetting(
          url, GURL(), ContentSettingsType::BRAVE_SHIELDS)) {
    return true;
  }

  // Check parent domains by iterating through all shield settings
  ContentSettingsForOneType all_shield_settings =
      host_content_settings_map_->GetSettingsForOneType(
          ContentSettingsType::BRAVE_SHIELDS);

  const auto ephemeral_domain = net::URLToEphemeralStorageDomain(url);

  for (const auto& setting : all_shield_settings) {
    // Skip invalid patterns or settings that don't disable shields
    if (!setting.primary_pattern.IsValid() ||
        setting.setting_value != CONTENT_SETTING_BLOCK) {
      continue;
    }

    // Skip wildcard patterns that match all hosts
    if (setting.primary_pattern.MatchesAllHosts()) {
      return true;
    }

    if (const GURL pattern_url(setting.primary_pattern.ToRepresentativeUrl());
        pattern_url.is_valid() &&
        ephemeral_domain == net::URLToEphemeralStorageDomain(pattern_url)) {
      return true;
    }
  }

  return false;
}

void BraveShieldsSettingsService::SetShredBrowsingHistory(bool value) {
  profile_prefs_->SetBoolean(prefs::kShredBrowsingHistoryEnabled, value);
}

bool BraveShieldsSettingsService::IsShredBrowsingHistoryEnabled() {
  return profile_prefs_->GetBoolean(prefs::kShredBrowsingHistoryEnabled);
}

bool BraveShieldsSettingsService::MakePseudoRandomGeneratorForURL(
    const GURL& url,
    base::span<const uint8_t> additional_entropy,
    FarblingPRNG* prng) {
  if (brave_shields::GetFarblingLevel(&*host_content_settings_map_, url) ==
      brave_shields::mojom::FarblingLevel::OFF) {
    return false;
  }
  const base::Token farbling_token = GetFarblingToken(url, additional_entropy);
  if (farbling_token.is_zero()) {
    return false;
  }
  *prng = FarblingPRNG(farbling_token.high() ^ farbling_token.low());
  return true;
}

// static
void BraveShieldsSettingsService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(brave_shields::prefs::kReduceLanguageEnabled,
                                true);
}

base::Token BraveShieldsSettingsService::GetFarblingToken(
    const GURL& url,
    base::span<const uint8_t> additional_entropy) {
  base::Token token;
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return token;
  }

  // Get the farbling token from the Shields metadata.
  auto shields_metadata = GetShieldsMetadata(&*host_content_settings_map_, url);
  if (auto* farbling_token = shields_metadata.FindString("farbling_token")) {
    token = base::Token::FromString(*farbling_token).value_or(base::Token());
  }

  // If the farbling token is not set or failed to parse, generate a new one.
  if (token.is_zero()) {
    if (!g_stable_farbling_tokens_seed) {
      token = base::Token::CreateRandom();
    } else {
      token = CreateStableFarblingToken(url);
    }
    shields_metadata.Set("farbling_token", token.ToString());
    SetShieldsMetadata(&*host_content_settings_map_, url,
                       std::move(shields_metadata));
  }

  if (additional_entropy.empty()) {
    return token;
  }

  const uint64_t high = token.high() ^ PersistentHashU64(additional_entropy);
  const uint64_t low =
      token.low() ^ PersistentHashU64(base::byte_span_from_ref(high));
  return base::Token(high, low);
}

}  // namespace brave_shields
