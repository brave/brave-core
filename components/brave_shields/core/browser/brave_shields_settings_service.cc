// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"

#include "base/hash/hash.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "net/base/features.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace brave_shields {

// Used for stable farbling token generation in tests when is set to non-zero.
// Non-anonymous to be accesible from ":test_support" target.
uint32_t g_stable_farbling_tokens_seed = 0;

namespace {

class BraveCookieRules {
 public:
  BraveCookieRules(ContentSetting general_setting,
                   ContentSetting first_party_setting)
      : general_setting_(general_setting),
        first_party_setting_(first_party_setting) {}

  ContentSetting general_setting() const { return general_setting_; }
  ContentSetting first_party_setting() const { return first_party_setting_; }

  static BraveCookieRules CalculateRules(
      HostContentSettingsMap* map,
      content_settings::CookieSettings* cookie_settings,
      const GURL& url) {
    const auto default_rules = BraveCookieRules::GetDefault(cookie_settings);
    if (IsDefaultCookieManaged(cookie_settings)) {
      return default_rules;
    }

    auto result = BraveCookieRules::Get(map, url);
    if (result.HasDefault()) {
      result.Merge(default_rules);
    }
    return result;
  }

 private:
  bool HasDefault() const {
    return general_setting_ == CONTENT_SETTING_DEFAULT ||
           first_party_setting_ == CONTENT_SETTING_DEFAULT;
  }

  static bool IsDefaultCookieManaged(
      content_settings::CookieSettings* cookie_settings) {
    content_settings::ProviderType provider_id;
    cookie_settings->GetDefaultCookieSetting(&provider_id);
    return provider_id == content_settings::ProviderType::kPolicyProvider;
  }

  static BraveCookieRules Get(HostContentSettingsMap* map, const GURL& url) {
    content_settings::SettingInfo general_info;
    const base::Value& general_value = map->GetWebsiteSetting(
        GURL::EmptyGURL(), url, ContentSettingsType::BRAVE_COOKIES,
        &general_info);

    content_settings::SettingInfo first_party_info;
    const base::Value& first_party_value = map->GetWebsiteSetting(
        url, url, ContentSettingsType::BRAVE_COOKIES, &first_party_info);

    const auto normalize_value = [](const content_settings::SettingInfo& info,
                                    const base::Value& value) {
      const ContentSettingsPattern& wildcard =
          ContentSettingsPattern::Wildcard();
      if (info.primary_pattern == wildcard &&
          info.secondary_pattern == wildcard) {
        return CONTENT_SETTING_DEFAULT;
      }
      return content_settings::ValueToContentSetting(value);
    };

    return {normalize_value(general_info, general_value),
            normalize_value(first_party_info, first_party_value)};
  }

  static BraveCookieRules GetDefault(
      content_settings::CookieSettings* cookie_settings) {
    const ContentSetting default_cookies_setting =
        cookie_settings->GetDefaultCookieSetting(nullptr);
    const bool default_should_block_3p_cookies =
        cookie_settings->ShouldBlockThirdPartyCookies();
    if (default_cookies_setting == CONTENT_SETTING_BLOCK) {
      // All cookies are blocked.
      return {CONTENT_SETTING_BLOCK, CONTENT_SETTING_BLOCK};
    } else if (default_should_block_3p_cookies) {
      // First-party cookies are allowed.
      return {CONTENT_SETTING_BLOCK, CONTENT_SETTING_ALLOW};
    }
    // All cookies are allowed.
    return {CONTENT_SETTING_ALLOW, CONTENT_SETTING_ALLOW};
  }

  void Merge(const BraveCookieRules& other) {
    if (general_setting_ == CONTENT_SETTING_DEFAULT) {
      general_setting_ = other.general_setting_;
    }
    if (first_party_setting_ == CONTENT_SETTING_DEFAULT) {
      first_party_setting_ = other.first_party_setting_;
    }
  }

 private:
  ContentSetting general_setting_ = CONTENT_SETTING_DEFAULT;
  ContentSetting first_party_setting_ = CONTENT_SETTING_DEFAULT;
};

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
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  if (url.is_empty()) {
    LOG(ERROR) << "url for shields setting cannot be blank";
    return;
  }

  auto primary_pattern = GetPatternFromURL(url);

  if (primary_pattern.MatchesAllHosts()) {
    LOG(ERROR) << "Url for shields setting cannot be blank or result in a "
                  "wildcard content setting.";

    DUMP_WILL_BE_NOTREACHED();
    return;
  }

  if (!primary_pattern.IsValid()) {
    DLOG(ERROR) << "Invalid primary pattern for Url: "
                << url.possibly_invalid_spec();
    return;
  }

  // this is 'allow_brave_shields' so 'enable' == 'allow'
  const auto setting =
      is_enabled ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;
  if (host_content_settings_map_->IsOffTheRecord() ||
      setting != host_content_settings_map_->GetDefaultContentSetting(
                     ContentSettingsType::BRAVE_SHIELDS, nullptr)) {
    host_content_settings_map_->SetContentSettingCustomScope(
        primary_pattern, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::BRAVE_SHIELDS, setting);

    if (!host_content_settings_map_->IsOffTheRecord()) {
      MaybeRecordShieldsUsageP3A(kShutOffShields, local_state_);
    }
  } else {
    host_content_settings_map_->SetContentSettingCustomScope(
        primary_pattern, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::BRAVE_SHIELDS, CONTENT_SETTING_DEFAULT);
  }
}

bool BraveShieldsSettingsService::GetBraveShieldsEnabled(const GURL& url) {
  if (base::FeatureList::IsEnabled(features::kBraveExtensionNetworkBlocking) &&
      url.SchemeIs(kChromeExtensionScheme)) {
    return true;
  }
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }

  ContentSetting setting = host_content_settings_map_->GetContentSetting(
      url, GURL(), ContentSettingsType::BRAVE_SHIELDS);

  // see EnableBraveShields - allow and default == true
  return setting == CONTENT_SETTING_BLOCK ? false : true;
}

void BraveShieldsSettingsService::SetDefaultAdBlockMode(
    mojom::AdBlockMode mode) {
  SetAdBlockMode(mode, GURL());
}

mojom::AdBlockMode BraveShieldsSettingsService::GetDefaultAdBlockMode() {
  return GetAdBlockMode(GURL());
}

void BraveShieldsSettingsService::SetFingerprintingControlType(
    ControlType type,
    const GURL& url) {
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  ControlType prev_setting = GetFingerprintingControlType(url);
  content_settings::SettingInfo setting_info;
  base::Value web_setting = host_content_settings_map_->GetWebsiteSetting(
      url, GURL(), ContentSettingsType::BRAVE_FINGERPRINTING_V2, &setting_info);
  bool was_default =
      web_setting.is_none() || setting_info.primary_pattern.MatchesAllHosts() ||
      setting_info.source == content_settings::SettingSource::kRemoteList;

  ContentSetting content_setting;
  if (type == ControlType::DEFAULT || type == ControlType::BLOCK_THIRD_PARTY) {
    type = ControlType::DEFAULT;
    content_setting = CONTENT_SETTING_ASK;
  } else {
    content_setting = GetDefaultBlockFromControlType(type);
  }

  host_content_settings_map_->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, content_setting);
  if (!host_content_settings_map_->IsOffTheRecord()) {
    // Only report to P3A if not a guest/incognito profile
    MaybeRecordShieldsUsageP3A(kChangedPerSiteShields, local_state_);
    if (url.is_empty()) {
      // If global setting changed, report global setting and recalulate
      // domain specific setting counts
      RecordShieldsFingerprintSetting(type);
      RecordShieldsDomainSettingCounts(profile_prefs_, true, type);
    } else {
      // If domain specific setting changed, recalculate counts
      ControlType global_setting = GetFingerprintingControlType(GURL());
      RecordShieldsDomainSettingCountsWithChange(
          profile_prefs_, true, global_setting,
          was_default ? nullptr : &prev_setting, type);
    }
  }
}
ControlType BraveShieldsSettingsService::GetFingerprintingControlType(
    const GURL& url) {
  ContentSettingsForOneType fingerprinting_rules =
      host_content_settings_map_->GetSettingsForOneType(
          ContentSettingsType::BRAVE_FINGERPRINTING_V2);

  ContentSetting fp_setting =
      GetBraveFPContentSettingFromRules(fingerprinting_rules, url);

  if (fp_setting == CONTENT_SETTING_ASK ||
      fp_setting == CONTENT_SETTING_DEFAULT ||
      (!IsShowStrictFingerprintingModeEnabled() &&
       fp_setting == CONTENT_SETTING_BLOCK)) {
    return ControlType::DEFAULT;
  }

  return fp_setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                             : ControlType::BLOCK;
}

mojom::FarblingLevel BraveShieldsSettingsService::GetFarblingLevel(
    const GURL& primary_url) {
  if (!base::FeatureList::IsEnabled(features::kBraveFarbling)) {
    return mojom::FarblingLevel::OFF;
  }

  const bool shields_up = GetBraveShieldsEnabled(primary_url);
  if (!shields_up) {
    return mojom::FarblingLevel::OFF;
  }

  auto fingerprinting_type = GetFingerprintingControlType(primary_url);
  switch (fingerprinting_type) {
    case ControlType::ALLOW:
      return mojom::FarblingLevel::OFF;
    case ControlType::BLOCK:
      return mojom::FarblingLevel::MAXIMUM;
    case ControlType::BLOCK_THIRD_PARTY:
      NOTREACHED();
    case ControlType::DEFAULT:
      return mojom::FarblingLevel::BALANCED;
  }
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

bool BraveShieldsSettingsService::IsReduceLanguageEnabledForProfile(
    PrefService* pref_service) {
  // Don't reduce language if feature is disabled
  if (!base::FeatureList::IsEnabled(features::kBraveReduceLanguage)) {
    return false;
  }

  // Don't reduce language if user preference is unchecked
  if (!pref_service->GetBoolean(prefs::kReduceLanguageEnabled)) {
    return false;
  }

  return true;
}

bool BraveShieldsSettingsService::ShouldDoReduceLanguage(const GURL& url) {
  if (!IsReduceLanguageEnabledForProfile(profile_prefs_)) {
    return false;
  }

  // Don't reduce language if Brave Shields is down (this also handles cases
  // where the URL is not HTTP(S))
  if (!GetBraveShieldsEnabled(url)) {
    return false;
  }

  // Don't reduce language if fingerprinting is off
  if (GetFingerprintingControlType(url) == ControlType::ALLOW) {
    return false;
  }

  // Don't reduce language if there's a webcompat exception
  if (IsWebcompatEnabled(&*host_content_settings_map_,
                         ContentSettingsType::BRAVE_WEBCOMPAT_LANGUAGE, url)) {
    return false;
  }

  return true;
}

void BraveShieldsSettingsService::SetCookieControlType(ControlType type,
                                                       const GURL& url) {
  auto patterns = content_settings::CreateShieldsCookiesPatterns(url);
  if (!patterns.host_pattern.IsValid()) {
    return;
  }

  MaybeRecordShieldsUsageP3A(kChangedPerSiteShields, local_state_);

  if (patterns.host_pattern == ContentSettingsPattern::Wildcard()) {
    // Default settings.
    switch (type) {
      case ControlType::ALLOW:
        host_content_settings_map_->SetDefaultContentSetting(
            ContentSettingsType::COOKIES, CONTENT_SETTING_ALLOW);
        profile_prefs_->SetInteger(
            ::prefs::kCookieControlsMode,
            static_cast<int>(content_settings::CookieControlsMode::kOff));
        break;
      case ControlType::BLOCK:
        host_content_settings_map_->SetDefaultContentSetting(
            ContentSettingsType::COOKIES, CONTENT_SETTING_BLOCK);
        // Toggle the state off->on to enforce the pref update event on switch
        // between BLOCK_THIRD_PARTY->BLOCK so the upstream Third-party cookies
        // Settings page can be updated correctly. This is a temporary measure
        // until we figure out a better UI for Cookies Settings page.
        profile_prefs_->SetInteger(
            ::prefs::kCookieControlsMode,
            static_cast<int>(content_settings::CookieControlsMode::kOff));
        profile_prefs_->SetInteger(
            ::prefs::kCookieControlsMode,
            static_cast<int>(
                content_settings::CookieControlsMode::kBlockThirdParty));
        break;
      case ControlType::BLOCK_THIRD_PARTY:
        host_content_settings_map_->SetDefaultContentSetting(
            ContentSettingsType::COOKIES, CONTENT_SETTING_ALLOW);
        // Toggle the state off->on to enforce the pref update event on switch
        // between BLOCK->BLOCK_THIRD_PARTY so the upstream Third-party cookies
        // Settings page can be updated correctly. This is a temporary measure
        // until we figure out a better UI for Cookies Settings page.
        profile_prefs_->SetInteger(
            ::prefs::kCookieControlsMode,
            static_cast<int>(content_settings::CookieControlsMode::kOff));
        profile_prefs_->SetInteger(
            ::prefs::kCookieControlsMode,
            static_cast<int>(
                content_settings::CookieControlsMode::kBlockThirdParty));
        break;
      case ControlType::DEFAULT:
        NOTREACHED() << "Invalid ControlType for cookies";
    }
    return;
  }

  host_content_settings_map_->SetContentSettingCustomScope(
      patterns.host_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_REFERRERS,
      GetDefaultBlockFromControlType(type));

  switch (type) {
    case ControlType::BLOCK_THIRD_PARTY:
      // general-rule:
      host_content_settings_map_->SetContentSettingCustomScope(
          ContentSettingsPattern::Wildcard(), patterns.host_pattern,
          ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_BLOCK);
      // first-party rule:
      host_content_settings_map_->SetContentSettingCustomScope(
          patterns.domain_pattern, patterns.host_pattern,
          ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_ALLOW);
      break;
    case ControlType::ALLOW:
    case ControlType::BLOCK:
      // Remove first-party rule:
      host_content_settings_map_->SetContentSettingCustomScope(
          patterns.domain_pattern, patterns.host_pattern,
          ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_DEFAULT);
      // general-rule:
      host_content_settings_map_->SetContentSettingCustomScope(
          ContentSettingsPattern::Wildcard(), patterns.host_pattern,
          ContentSettingsType::BRAVE_COOKIES,
          (type == ControlType::ALLOW) ? CONTENT_SETTING_ALLOW
                                       : CONTENT_SETTING_BLOCK);
      break;
    case ControlType::DEFAULT:
      NOTREACHED() << "Invalid ControlType for cookies";
  }
}

ControlType BraveShieldsSettingsService::GetCookieControlType(
    content_settings::CookieSettings* cookie_settings,
    const GURL& url) {
  DCHECK(cookie_settings);

  auto result = BraveCookieRules::CalculateRules(&*host_content_settings_map_,
                                                 cookie_settings, url);

  if (result.general_setting() == CONTENT_SETTING_ALLOW) {
    return ControlType::ALLOW;
  }
  if (result.first_party_setting() != CONTENT_SETTING_BLOCK) {
    return ControlType::BLOCK_THIRD_PARTY;
  }
  return ControlType::BLOCK;
}

void BraveShieldsSettingsService::SetNoScriptControlType(ControlType type,
                                                         const GURL& url) {
  DCHECK_NE(type, ControlType::BLOCK_THIRD_PARTY);
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  host_content_settings_map_->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT,
      type == ControlType::ALLOW ? CONTENT_SETTING_ALLOW
                                 : CONTENT_SETTING_BLOCK);
  MaybeRecordShieldsUsageP3A(kChangedPerSiteShields, local_state_);
}

ControlType BraveShieldsSettingsService::GetNoScriptControlType(
    const GURL& url) {
  ContentSetting setting = host_content_settings_map_->GetContentSetting(
      url, GURL(), ContentSettingsType::JAVASCRIPT);

  return setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                          : ControlType::BLOCK;
}

void BraveShieldsSettingsService::SetHttpsUpgradeControlType(ControlType type,
                                                             const GURL& url) {
  DCHECK_NE(type, ControlType::DEFAULT);
  if (!url.SchemeIsHTTPOrHTTPS() && !url.is_empty()) {
    return;
  }

  auto primary_pattern = GetPatternFromURL(url);
  if (!primary_pattern.IsValid()) {
    return;
  }

  ContentSetting setting;
  if (type == ControlType::ALLOW) {
    // Allow http connections
    setting = CONTENT_SETTING_ALLOW;
  } else if (type == ControlType::BLOCK) {
    // Require https
    setting = CONTENT_SETTING_BLOCK;
  } else if (type == ControlType::BLOCK_THIRD_PARTY) {
    // Prefer https
    setting = CONTENT_SETTING_ASK;
  } else {
    // Fall back to default
    setting = CONTENT_SETTING_DEFAULT;
  }
  host_content_settings_map_->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_HTTPS_UPGRADE, setting);

  // Reset the HTTPS fallback map.
  if (url.is_empty()) {
    host_content_settings_map_->ClearSettingsForOneType(
        ContentSettingsType::HTTP_ALLOWED);
  } else {
    const GURL& secure_url = GURL(base::StrCat({"https://", url.host()}));
    host_content_settings_map_->SetWebsiteSettingDefaultScope(
        secure_url, GURL(), ContentSettingsType::HTTP_ALLOWED, base::Value());
  }

  MaybeRecordShieldsUsageP3A(kChangedPerSiteShields, local_state_);
  RecordHTTPSUpgradeSettingP3A(&*host_content_settings_map_);
}

ControlType BraveShieldsSettingsService::GetHttpsUpgradeControlType(
    const GURL& url) {
  if (!url.SchemeIsHTTPOrHTTPS() && !url.is_empty()) {
    // No upgrades happen for non-http(s) URLs.
    return ControlType::ALLOW;
  }
  ContentSetting setting = host_content_settings_map_->GetContentSetting(
      url, GURL(), ContentSettingsType::BRAVE_HTTPS_UPGRADE);
  if (setting == CONTENT_SETTING_ALLOW) {
    // Disabled (allow http)
    return ControlType::ALLOW;
  } else if (setting == CONTENT_SETTING_BLOCK) {
    // HTTPS Only (require https)
    return ControlType::BLOCK;
  } else if (setting == CONTENT_SETTING_ASK) {
    // HTTPS Only (prefer https)
    return ControlType::BLOCK_THIRD_PARTY;
  } else {
    // HTTPS by default (upgrade when available)
    return ControlType::DEFAULT;
  }
}

bool BraveShieldsSettingsService::ShouldUpgradeToHttps(
    const GURL& url,
    https_upgrade_exceptions::HttpsUpgradeExceptionsService*
        https_upgrade_exceptions_service) {
  // Don't upgrade if we don't have an exceptions service.
  if (!https_upgrade_exceptions_service) {
    return false;
  }
  // Don't upgrade if feature is disabled.
  if (!base::FeatureList::IsEnabled(net::features::kBraveHttpsByDefault)) {
    return false;
  }
  if (!url.SchemeIsHTTPOrHTTPS() && !url.is_empty()) {
    return false;
  }
  // Don't upgrade if shields are down.
  if (!GetBraveShieldsEnabled(url)) {
    return false;
  }
  const ControlType control_type = GetHttpsUpgradeControlType(url);
  // Always upgrade for Strict HTTPS Upgrade.
  if (control_type == ControlType::BLOCK) {
    return true;
  }
  // Upgrade for Standard HTTPS upgrade if host is not on the exceptions list.
  if (control_type == ControlType::BLOCK_THIRD_PARTY &&
      https_upgrade_exceptions_service->CanUpgradeToHTTPS(url)) {
    return true;
  }
  return false;
}

bool BraveShieldsSettingsService::ShouldForceHttps(const GURL& url) {
  return GetBraveShieldsEnabled(url) &&
         GetHttpsUpgradeControlType(url) == ControlType::BLOCK;
}

void BraveShieldsSettingsService::SetAdControlType(ControlType type,
                                                   const GURL& url) {
  DCHECK_NE(type, ControlType::BLOCK_THIRD_PARTY);
  DCHECK_NE(type, ControlType::DEFAULT);
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  host_content_settings_map_->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_ADS, GetDefaultBlockFromControlType(type));

  host_content_settings_map_->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_TRACKERS,
      GetDefaultBlockFromControlType(type));

  MaybeRecordShieldsUsageP3A(kChangedPerSiteShields, local_state_);
}

ControlType BraveShieldsSettingsService::GetAdControlType(const GURL& url) {
  if (base::FeatureList::IsEnabled(features::kBraveExtensionNetworkBlocking) &&
      url.SchemeIs(kChromeExtensionScheme)) {
    return ControlType::BLOCK;
  }
  ContentSetting setting = host_content_settings_map_->GetContentSetting(
      url, GURL(), ContentSettingsType::BRAVE_ADS);

  return setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                          : ControlType::BLOCK;
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

  SetAdControlType(control_type_ad, url);

  SetCosmeticFilteringControlType(control_type_cosmetic, url);
}

mojom::AdBlockMode BraveShieldsSettingsService::GetAdBlockMode(
    const GURL& url) {
  ControlType control_type_ad = GetAdControlType(url);

  ControlType control_type_cosmetic = GetCosmeticFilteringControlType(url);

  if (control_type_ad == ControlType::ALLOW) {
    return mojom::AdBlockMode::ALLOW;
  }

  if (control_type_cosmetic == ControlType::BLOCK) {
    return mojom::AdBlockMode::AGGRESSIVE;
  } else {
    return mojom::AdBlockMode::STANDARD;
  }
}

void BraveShieldsSettingsService::SetCosmeticFilteringControlType(
    ControlType type,
    const GURL& url) {
  DCHECK_NE(type, ControlType::DEFAULT);
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  ControlType prev_setting = GetCosmeticFilteringControlType(url);
  content_settings::SettingInfo setting_info;
  base::Value web_setting = host_content_settings_map_->GetWebsiteSetting(
      url, GURL::EmptyGURL(), CosmeticFilteringSetting::kContentSettingsType,
      &setting_info);
  bool was_default =
      web_setting.is_none() || setting_info.primary_pattern.MatchesAllHosts();

  host_content_settings_map_->SetWebsiteSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      CosmeticFilteringSetting::kContentSettingsType,
      CosmeticFilteringSetting::ToValue(type));

  if (!host_content_settings_map_->IsOffTheRecord()) {
    // Only report to P3A if not a guest/incognito profile
    MaybeRecordShieldsUsageP3A(kChangedPerSiteShields, local_state_);
    if (url.is_empty()) {
      // If global setting changed, report global setting and recalulate
      // domain specific setting counts
      RecordShieldsAdsSetting(type);
      RecordShieldsDomainSettingCounts(profile_prefs_, false, type);
    } else {
      // If domain specific setting changed, recalculate counts
      ControlType global_setting = GetCosmeticFilteringControlType(GURL());
      RecordShieldsDomainSettingCountsWithChange(
          profile_prefs_, false, global_setting,
          was_default ? nullptr : &prev_setting, type);
    }
  }
}

ControlType BraveShieldsSettingsService::GetCosmeticFilteringControlType(
    const GURL& url) {
  if (base::FeatureList::IsEnabled(features::kBraveExtensionNetworkBlocking) &&
      url.SchemeIs(kChromeExtensionScheme)) {
    return ControlType::BLOCK;
  }
  const auto setting = CosmeticFilteringSetting::FromValue(
      host_content_settings_map_->GetWebsiteSetting(
          url, GURL::EmptyGURL(),
          CosmeticFilteringSetting::kContentSettingsType));
  return setting;
}

bool BraveShieldsSettingsService::IsFirstPartyCosmeticFilteringEnabled(
    const GURL& url) {
  const ControlType type = GetCosmeticFilteringControlType(url);
  return type == ControlType::BLOCK;
}

DomainBlockingType BraveShieldsSettingsService::GetDomainBlockingType(
    const GURL& url) {
  // Don't block if feature is disabled
  if (!base::FeatureList::IsEnabled(features::kBraveDomainBlock)) {
    return DomainBlockingType::kNone;
  }

  // Don't block if Brave Shields is down (this also handles cases where
  // the URL is not HTTP(S))
  if (!GetBraveShieldsEnabled(url)) {
    return DomainBlockingType::kNone;
  }

  // Don't block if ad blocking is off.
  if (GetAdControlType(url) != ControlType::BLOCK) {
    return DomainBlockingType::kNone;
  }

  const ControlType cosmetic_control_type =
      GetCosmeticFilteringControlType(url);
  // Block if ad blocking is "aggressive".
  if (cosmetic_control_type == ControlType::BLOCK) {
    return DomainBlockingType::kAggressive;
  }

  // Block using 1PES if ad blocking is "standard".
  if (cosmetic_control_type == BLOCK_THIRD_PARTY &&
      base::FeatureList::IsEnabled(
          net::features::kBraveFirstPartyEphemeralStorage) &&
      base::FeatureList::IsEnabled(features::kBraveDomainBlock1PES)) {
    return DomainBlockingType::k1PES;
  }

  return DomainBlockingType::kNone;
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

  SetFingerprintingControlType(control_type, url);
}

mojom::FingerprintMode BraveShieldsSettingsService::GetFingerprintMode(
    const GURL& url) {
  ControlType control_type = GetFingerprintingControlType(url);

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
  SetNoScriptControlType(control_type, url);
}

bool BraveShieldsSettingsService::IsNoScriptEnabled(const GURL& url) {
  ControlType control_type = GetNoScriptControlType(url);

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
  CHECK(base::FeatureList::IsEnabled(features::kBraveShredFeature));

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
  CHECK(base::FeatureList::IsEnabled(features::kBraveShredFeature));
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

}  // namespace brave_shields
