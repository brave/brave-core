// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"

#include <string>
#include <utility>

#include "base/debug/dump_without_crashing.h"
#include "base/hash/hash.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "net/base/features.h"
#include "url/gurl.h"

namespace brave_shields {

// Used for stable farbling token generation in tests when is set to non-zero.
// Non-anonymous to be accesible from ":test_support" target.
uint32_t g_stable_farbling_tokens_seed = 0;

namespace {

void RecordShieldsToggled(PrefService* local_state) {
  ::brave_shields::MaybeRecordShieldsUsageP3A(::brave_shields::kShutOffShields,
                                              local_state);
}

void RecordShieldsSettingChanged(PrefService* local_state) {
  ::brave_shields::MaybeRecordShieldsUsageP3A(
      ::brave_shields::kChangedPerSiteShields, local_state);
}

ContentSetting GetDefaultBlockFromControlType(ControlType type) {
  if (type == ControlType::DEFAULT) {
    return CONTENT_SETTING_DEFAULT;
  }

  return type == ControlType::ALLOW ? CONTENT_SETTING_ALLOW
                                    : CONTENT_SETTING_BLOCK;
}

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

base::Value::Dict GetShieldsMetadata(HostContentSettingsMap* map,
                                     const GURL& url) {
  auto shields_metadata_value = map->GetWebsiteSetting(
      url, url, ContentSettingsType::BRAVE_SHIELDS_METADATA);
  if (auto* shields_metadata_dict = shields_metadata_value.GetIfDict()) {
    return std::move(*shields_metadata_dict);
  }
  return base::Value::Dict();
}

void SetShieldsMetadata(HostContentSettingsMap* map,
                        const GURL& url,
                        base::Value::Dict shields_metadata) {
  map->SetWebsiteSettingDefaultScope(
      url, url, ContentSettingsType::BRAVE_SHIELDS_METADATA,
      base::Value(std::move(shields_metadata)));
}

base::Token CreateStableFarblingToken(const GURL& url) {
  const uint32_t high = base::PersistentHash(url.host_piece()) +
                        g_stable_farbling_tokens_seed - 1;
  const uint32_t low = base::PersistentHash(base::byte_span_from_ref(high));
  return base::Token(high, low);
}

}  // namespace

ContentSettingsPattern GetPatternFromURL(const GURL& url) {
  return content_settings::CreateHostPattern(url);
}

std::string ControlTypeToString(ControlType type) {
  switch (type) {
    case ControlType::ALLOW:
      return "allow";
    case ControlType::BLOCK:
      return "block";
    case ControlType::BLOCK_THIRD_PARTY:
      return "block_third_party";
    case ControlType::DEFAULT:
      return "default";
  }
  NOTREACHED() << "Unexpected value for ControlType: "
               << base::to_underlying(type);
}

ControlType ControlTypeFromString(const std::string& string) {
  if (string == "allow") {
    return ControlType::ALLOW;
  } else if (string == "block") {
    return ControlType::BLOCK;
  } else if (string == "block_third_party") {
    return ControlType::BLOCK_THIRD_PARTY;
  } else if (string == "default") {
    return ControlType::DEFAULT;
  }
  NOTREACHED();
}

void SetBraveShieldsEnabled(HostContentSettingsMap* map,
                            bool enable,
                            const GURL& url,
                            PrefService* local_state) {
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

#if DCHECK_IS_ON()
    NOTREACHED();
#else
    base::debug::DumpWithoutCrashing();
    return;
#endif
  }

  if (!primary_pattern.IsValid()) {
    DLOG(ERROR) << "Invalid primary pattern for Url: "
                << url.possibly_invalid_spec();
    return;
  }

  // this is 'allow_brave_shields' so 'enable' == 'allow'
  const auto setting = enable ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;
  if (map->IsOffTheRecord() ||
      setting != map->GetDefaultContentSetting(
                     ContentSettingsType::BRAVE_SHIELDS, nullptr)) {
    map->SetContentSettingCustomScope(
        primary_pattern, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::BRAVE_SHIELDS, setting);

    if (!map->IsOffTheRecord()) {
      RecordShieldsToggled(local_state);
    }
  } else {
    map->SetContentSettingCustomScope(
        primary_pattern, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::BRAVE_SHIELDS, CONTENT_SETTING_DEFAULT);
  }
}

bool GetBraveShieldsEnabled(HostContentSettingsMap* map, const GURL& url) {
  if (base::FeatureList::IsEnabled(
          ::brave_shields::features::kBraveExtensionNetworkBlocking) &&
      url.SchemeIs(kChromeExtensionScheme)) {
    return true;
  }
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }

  ContentSetting setting =
      map->GetContentSetting(url, GURL(), ContentSettingsType::BRAVE_SHIELDS);

  // see EnableBraveShields - allow and default == true
  return setting == CONTENT_SETTING_BLOCK ? false : true;
}

void SetAdControlType(HostContentSettingsMap* map,
                      ControlType type,
                      const GURL& url,
                      PrefService* local_state) {
  DCHECK_NE(type, ControlType::BLOCK_THIRD_PARTY);
  DCHECK_NE(type, ControlType::DEFAULT);
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_ADS, GetDefaultBlockFromControlType(type));

  map->SetContentSettingCustomScope(primary_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::BRAVE_TRACKERS,
                                    GetDefaultBlockFromControlType(type));
  RecordShieldsSettingChanged(local_state);
}

ControlType GetAdControlType(HostContentSettingsMap* map, const GURL& url) {
  if (base::FeatureList::IsEnabled(
          ::brave_shields::features::kBraveExtensionNetworkBlocking) &&
      url.SchemeIs(kChromeExtensionScheme)) {
    return ControlType::BLOCK;
  }
  ContentSetting setting =
      map->GetContentSetting(url, GURL(), ContentSettingsType::BRAVE_ADS);

  return setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                          : ControlType::BLOCK;
}

void SetCosmeticFilteringControlType(HostContentSettingsMap* map,
                                     ControlType type,
                                     const GURL& url,
                                     PrefService* local_state,
                                     PrefService* profile_state) {
  DCHECK_NE(type, ControlType::DEFAULT);
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  ControlType prev_setting = GetCosmeticFilteringControlType(map, url);
  content_settings::SettingInfo setting_info;
  base::Value web_setting = map->GetWebsiteSetting(
      url, GURL::EmptyGURL(), CosmeticFilteringSetting::kContentSettingsType,
      &setting_info);
  bool was_default =
      web_setting.is_none() || setting_info.primary_pattern.MatchesAllHosts();

  map->SetWebsiteSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      CosmeticFilteringSetting::kContentSettingsType,
      CosmeticFilteringSetting::ToValue(type));

  if (!map->IsOffTheRecord()) {
    // Only report to P3A if not a guest/incognito profile
    RecordShieldsSettingChanged(local_state);
    if (url.is_empty()) {
      // If global setting changed, report global setting and recalulate
      // domain specific setting counts
      RecordShieldsAdsSetting(type);
      RecordShieldsDomainSettingCounts(profile_state, false, type);
    } else {
      // If domain specific setting changed, recalculate counts
      ControlType global_setting = GetCosmeticFilteringControlType(map, GURL());
      RecordShieldsDomainSettingCountsWithChange(
          profile_state, false, global_setting,
          was_default ? nullptr : &prev_setting, type);
    }
  }
}

ControlType GetCosmeticFilteringControlType(HostContentSettingsMap* map,
                                            const GURL& url) {
  if (base::FeatureList::IsEnabled(
          ::brave_shields::features::kBraveExtensionNetworkBlocking) &&
      url.SchemeIs(kChromeExtensionScheme)) {
    return ControlType::BLOCK;
  }
  const auto setting = CosmeticFilteringSetting::FromValue(
      map->GetWebsiteSetting(url, GURL::EmptyGURL(),
                             CosmeticFilteringSetting::kContentSettingsType));
  return setting;
}

bool IsFirstPartyCosmeticFilteringEnabled(HostContentSettingsMap* map,
                                          const GURL& url) {
  const ControlType type = GetCosmeticFilteringControlType(map, url);
  return type == ControlType::BLOCK;
}

bool IsReduceLanguageEnabledForProfile(PrefService* pref_service) {
  // Don't reduce language if feature is disabled
  if (!base::FeatureList::IsEnabled(features::kBraveReduceLanguage)) {
    return false;
  }

  // Don't reduce language if user preference is unchecked
  if (!pref_service->GetBoolean(brave_shields::prefs::kReduceLanguageEnabled)) {
    return false;
  }

  return true;
}

bool ShouldDoReduceLanguage(HostContentSettingsMap* map,
                            const GURL& url,
                            PrefService* pref_service) {
  if (!IsReduceLanguageEnabledForProfile(pref_service)) {
    return false;
  }

  // Don't reduce language if Brave Shields is down (this also handles cases
  // where the URL is not HTTP(S))
  if (!brave_shields::GetBraveShieldsEnabled(map, url)) {
    return false;
  }

  // Don't reduce language if fingerprinting is off
  if (brave_shields::GetFingerprintingControlType(map, url) ==
      ControlType::ALLOW) {
    return false;
  }

  // Don't reduce language if there's a webcompat exception
  if (brave_shields::IsWebcompatEnabled(
          map, ContentSettingsType::BRAVE_WEBCOMPAT_LANGUAGE, url)) {
    return false;
  }

  return true;
}

DomainBlockingType GetDomainBlockingType(HostContentSettingsMap* map,
                                         const GURL& url) {
  // Don't block if feature is disabled
  if (!base::FeatureList::IsEnabled(
          brave_shields::features::kBraveDomainBlock)) {
    return DomainBlockingType::kNone;
  }

  // Don't block if Brave Shields is down (this also handles cases where
  // the URL is not HTTP(S))
  if (!brave_shields::GetBraveShieldsEnabled(map, url)) {
    return DomainBlockingType::kNone;
  }

  // Don't block if ad blocking is off.
  if (brave_shields::GetAdControlType(map, url) != ControlType::BLOCK) {
    return DomainBlockingType::kNone;
  }

  const ControlType cosmetic_control_type =
      brave_shields::GetCosmeticFilteringControlType(map, url);
  // Block if ad blocking is "aggressive".
  if (cosmetic_control_type == ControlType::BLOCK) {
    return DomainBlockingType::kAggressive;
  }

  // Block using 1PES if ad blocking is "standard".
  if (cosmetic_control_type == BLOCK_THIRD_PARTY &&
      base::FeatureList::IsEnabled(
          net::features::kBraveFirstPartyEphemeralStorage) &&
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveDomainBlock1PES)) {
    return DomainBlockingType::k1PES;
  }

  return DomainBlockingType::kNone;
}

void SetCookieControlType(HostContentSettingsMap* map,
                          PrefService* profile_state,
                          ControlType type,
                          const GURL& url,
                          PrefService* local_state) {
  auto patterns = content_settings::CreateShieldsCookiesPatterns(url);
  if (!patterns.host_pattern.IsValid()) {
    return;
  }

  RecordShieldsSettingChanged(local_state);

  if (patterns.host_pattern == ContentSettingsPattern::Wildcard()) {
    // Default settings.
    switch (type) {
      case ControlType::ALLOW:
        map->SetDefaultContentSetting(ContentSettingsType::COOKIES,
                                      CONTENT_SETTING_ALLOW);
        profile_state->SetInteger(
            ::prefs::kCookieControlsMode,
            static_cast<int>(content_settings::CookieControlsMode::kOff));
        break;
      case ControlType::BLOCK:
        map->SetDefaultContentSetting(ContentSettingsType::COOKIES,
                                      CONTENT_SETTING_BLOCK);
        // Toggle the state off->on to enforce the pref update event on switch
        // between BLOCK_THIRD_PARTY->BLOCK so the upstream Third-party cookies
        // Settings page can be updated correctly. This is a temporary measure
        // until we figure out a better UI for Cookies Settings page.
        profile_state->SetInteger(
            ::prefs::kCookieControlsMode,
            static_cast<int>(content_settings::CookieControlsMode::kOff));
        profile_state->SetInteger(
            ::prefs::kCookieControlsMode,
            static_cast<int>(
                content_settings::CookieControlsMode::kBlockThirdParty));
        break;
      case ControlType::BLOCK_THIRD_PARTY:
        map->SetDefaultContentSetting(ContentSettingsType::COOKIES,
                                      CONTENT_SETTING_ALLOW);
        // Toggle the state off->on to enforce the pref update event on switch
        // between BLOCK->BLOCK_THIRD_PARTY so the upstream Third-party cookies
        // Settings page can be updated correctly. This is a temporary measure
        // until we figure out a better UI for Cookies Settings page.
        profile_state->SetInteger(
            ::prefs::kCookieControlsMode,
            static_cast<int>(content_settings::CookieControlsMode::kOff));
        profile_state->SetInteger(
            ::prefs::kCookieControlsMode,
            static_cast<int>(
                content_settings::CookieControlsMode::kBlockThirdParty));
        break;
      case ControlType::DEFAULT:
        NOTREACHED() << "Invalid ControlType for cookies";
    }
    return;
  }

  map->SetContentSettingCustomScope(patterns.host_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::BRAVE_REFERRERS,
                                    GetDefaultBlockFromControlType(type));

  switch (type) {
    case ControlType::BLOCK_THIRD_PARTY:
      // general-rule:
      map->SetContentSettingCustomScope(
          ContentSettingsPattern::Wildcard(), patterns.host_pattern,
          ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_BLOCK);
      // first-party rule:
      map->SetContentSettingCustomScope(
          patterns.domain_pattern, patterns.host_pattern,
          ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_ALLOW);
      break;
    case ControlType::ALLOW:
    case ControlType::BLOCK:
      // Remove first-party rule:
      map->SetContentSettingCustomScope(
          patterns.domain_pattern, patterns.host_pattern,
          ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_DEFAULT);
      // general-rule:
      map->SetContentSettingCustomScope(
          ContentSettingsPattern::Wildcard(), patterns.host_pattern,
          ContentSettingsType::BRAVE_COOKIES,
          (type == ControlType::ALLOW) ? CONTENT_SETTING_ALLOW
                                       : CONTENT_SETTING_BLOCK);
      break;
    case ControlType::DEFAULT:
      NOTREACHED() << "Invalid ControlType for cookies";
  }
}

ControlType GetCookieControlType(
    HostContentSettingsMap* map,
    content_settings::CookieSettings* cookie_settings,
    const GURL& url) {
  DCHECK(map);
  DCHECK(cookie_settings);

  auto result = BraveCookieRules::CalculateRules(map, cookie_settings, url);

  if (result.general_setting() == CONTENT_SETTING_ALLOW) {
    return ControlType::ALLOW;
  }
  if (result.first_party_setting() != CONTENT_SETTING_BLOCK) {
    return ControlType::BLOCK_THIRD_PARTY;
  }
  return ControlType::BLOCK;
}

void SetFingerprintingControlType(HostContentSettingsMap* map,
                                  ControlType type,
                                  const GURL& url,
                                  PrefService* local_state,
                                  PrefService* profile_state) {
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  ControlType prev_setting = GetFingerprintingControlType(map, url);
  content_settings::SettingInfo setting_info;
  base::Value web_setting = map->GetWebsiteSetting(
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

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, content_setting);
  if (!map->IsOffTheRecord()) {
    // Only report to P3A if not a guest/incognito profile
    RecordShieldsSettingChanged(local_state);
    if (url.is_empty()) {
      // If global setting changed, report global setting and recalulate
      // domain specific setting counts
      RecordShieldsFingerprintSetting(type);
      RecordShieldsDomainSettingCounts(profile_state, true, type);
    } else {
      // If domain specific setting changed, recalculate counts
      ControlType global_setting = GetFingerprintingControlType(map, GURL());
      RecordShieldsDomainSettingCountsWithChange(
          profile_state, true, global_setting,
          was_default ? nullptr : &prev_setting, type);
    }
  }
}

ControlType GetFingerprintingControlType(HostContentSettingsMap* map,
                                         const GURL& url) {
  ContentSettingsForOneType fingerprinting_rules =
      map->GetSettingsForOneType(ContentSettingsType::BRAVE_FINGERPRINTING_V2);

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

bool IsBraveShieldsManaged(PrefService* prefs,
                           HostContentSettingsMap* map,
                           GURL url) {
  DCHECK(prefs);
  DCHECK(map);
  content_settings::SettingInfo info;
  map->GetWebsiteSetting(url, url, ContentSettingsType::BRAVE_SHIELDS, &info);
  return info.source == content_settings::SettingSource::kPolicy;
}

bool IsShowStrictFingerprintingModeEnabled() {
  return base::FeatureList::IsEnabled(
      features::kBraveShowStrictFingerprintingMode);
}

void SetHttpsUpgradeControlType(HostContentSettingsMap* map,
                                ControlType type,
                                const GURL& url,
                                PrefService* local_state) {
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
  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_HTTPS_UPGRADE, setting);

  // Reset the HTTPS fallback map.
  if (url.is_empty()) {
    map->ClearSettingsForOneType(ContentSettingsType::HTTP_ALLOWED);
  } else {
    const GURL& secure_url = GURL("https://" + url.host());
    map->SetWebsiteSettingDefaultScope(
        secure_url, GURL(), ContentSettingsType::HTTP_ALLOWED, base::Value());
  }

  RecordShieldsSettingChanged(local_state);
}

ControlType GetHttpsUpgradeControlType(HostContentSettingsMap* map,
                                       const GURL& url) {
  if (!url.SchemeIsHTTPOrHTTPS() && !url.is_empty()) {
    // No upgrades happen for non-http(s) URLs.
    return ControlType::ALLOW;
  }
  ContentSetting setting = map->GetContentSetting(
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

bool ShouldUpgradeToHttps(
    HostContentSettingsMap* map,
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
  if (!GetBraveShieldsEnabled(map, url)) {
    return false;
  }
  const ControlType control_type = GetHttpsUpgradeControlType(map, url);
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

bool ShouldForceHttps(HostContentSettingsMap* map, const GURL& url) {
  return GetBraveShieldsEnabled(map, url) &&
         GetHttpsUpgradeControlType(map, url) == ControlType::BLOCK;
}

void SetNoScriptControlType(HostContentSettingsMap* map,
                            ControlType type,
                            const GURL& url,
                            PrefService* local_state) {
  DCHECK_NE(type, ControlType::BLOCK_THIRD_PARTY);
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT,
      type == ControlType::ALLOW ? CONTENT_SETTING_ALLOW
                                 : CONTENT_SETTING_BLOCK);
  RecordShieldsSettingChanged(local_state);
}

ControlType GetNoScriptControlType(HostContentSettingsMap* map,
                                   const GURL& url) {
  ContentSetting setting =
      map->GetContentSetting(url, GURL(), ContentSettingsType::JAVASCRIPT);

  return setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                          : ControlType::BLOCK;
}

void SetForgetFirstPartyStorageEnabled(HostContentSettingsMap* map,
                                       bool is_enabled,
                                       const GURL& url,
                                       PrefService* local_state) {
  auto primary_pattern = content_settings::CreateDomainPattern(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
      is_enabled ? CONTENT_SETTING_BLOCK : CONTENT_SETTING_ALLOW);
  RecordShieldsSettingChanged(local_state);
  RecordForgetFirstPartySetting(map);
}

bool GetForgetFirstPartyStorageEnabled(HostContentSettingsMap* map,
                                       const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE);

  return setting == CONTENT_SETTING_BLOCK;
}

void SetWebcompatEnabled(HostContentSettingsMap* map,
                         ContentSettingsType webcompat_settings_type,
                         bool enabled,
                         const GURL& url,
                         PrefService* local_state) {
  DCHECK(map);

  if (!url.SchemeIsHTTPOrHTTPS() && !url.is_empty()) {
    return;
  }

  auto primary_pattern = GetPatternFromURL(url);
  if (!primary_pattern.IsValid()) {
    return;
  }

  ContentSetting setting =
      enabled ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;
  map->SetContentSettingCustomScope(primary_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    webcompat_settings_type, setting);
  RecordShieldsSettingChanged(local_state);
}

bool IsWebcompatEnabled(HostContentSettingsMap* map,
                        ContentSettingsType webcompat_settings_type,
                        const GURL& url) {
  DCHECK(map);

  if (!url.SchemeIsHTTPOrHTTPS() && !url.is_empty()) {
    return false;
  }

  ContentSetting setting =
      map->GetContentSetting(url, url, webcompat_settings_type);

  return setting == CONTENT_SETTING_ALLOW;
}

mojom::FarblingLevel GetFarblingLevel(HostContentSettingsMap* map,
                                      const GURL& primary_url) {
  if (!base::FeatureList::IsEnabled(features::kBraveFarbling)) {
    return brave_shields::mojom::FarblingLevel::OFF;
  }

  const bool shields_up = GetBraveShieldsEnabled(map, primary_url);
  if (!shields_up) {
    return brave_shields::mojom::FarblingLevel::OFF;
  }

  auto fingerprinting_type = GetFingerprintingControlType(map, primary_url);
  switch (fingerprinting_type) {
    case ControlType::ALLOW:
      return brave_shields::mojom::FarblingLevel::OFF;
    case ControlType::BLOCK:
      return brave_shields::mojom::FarblingLevel::MAXIMUM;
    case ControlType::BLOCK_THIRD_PARTY:
      NOTREACHED();
    case ControlType::DEFAULT:
      return brave_shields::mojom::FarblingLevel::BALANCED;
  }
}

base::Token GetFarblingToken(HostContentSettingsMap* map, const GURL& url) {
  base::Token token;
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return token;
  }

  // Get the farbling token from the Shields metadata.
  auto shields_metadata = GetShieldsMetadata(map, url);
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
    SetShieldsMetadata(map, url, std::move(shields_metadata));
  }

  return token;
}

bool IsDeveloperModeEnabled(PrefService* profile_state) {
  return profile_state->GetBoolean(prefs::kAdBlockDeveloperMode);
}

}  // namespace brave_shields
