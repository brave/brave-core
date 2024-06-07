/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/common/cookie_settings_base.h"

#include <optional>

#include "base/auto_reset.h"
#include "base/compiler_specific.h"
#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/types/optional_util.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/features.h"
#include "net/base/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/cookies/site_for_cookies.h"
#include "third_party/abseil-cpp/absl/base/attributes.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content_settings {

namespace {

constexpr char kWp[] = "https://[*.]wp.com/*";
constexpr char kWordpress[] = "https://[*.]wordpress.com/*";

// The thread local brave metadata pointer.
ABSL_CONST_INIT thread_local CookieSettingWithBraveMetadata*
    current_cookie_settings_with_brave_metadata = nullptr;

bool BraveIsAllowedThirdParty(const GURL& url,
                              const GURL& first_party_url,
                              const CookieSettingsBase* const cookie_settings) {
  static const base::NoDestructor<
      // url -> first_party_url allow map
      std::vector<std::pair<ContentSettingsPattern, ContentSettingsPattern>>>
      entity_list({{ContentSettingsPattern::FromString(kWp),
                    ContentSettingsPattern::FromString(kWordpress)},
                   {ContentSettingsPattern::FromString(kWordpress),
                    ContentSettingsPattern::FromString(kWp)}});

  if (net::registry_controlled_domains::GetDomainAndRegistry(
          url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES) ==
      net::registry_controlled_domains::GetDomainAndRegistry(
          first_party_url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES))
    return true;

  for (auto i = entity_list->begin(); i != entity_list->end(); ++i) {
    if (i->first.Matches(url) && i->second.Matches(first_party_url))
      return true;
  }

  return false;
}

bool IsFirstPartyAccessAllowed(const GURL& first_party_url,
                               const CookieSettingsBase* const cookie_settings,
                               net::CookieSettingOverrides overrides) {
  ContentSetting setting = cookie_settings->GetCookieSetting(
      first_party_url, first_party_url, overrides, nullptr);
  return cookie_settings->IsAllowed(setting);
}

bool IsSessionOnlyExplicit(
    const CookieSettingWithBraveMetadata& setting_with_brave_metadata) {
  return setting_with_brave_metadata.setting == CONTENT_SETTING_SESSION_ONLY &&
         setting_with_brave_metadata.IsExplicitSetting();
}

}  // namespace

CookieSettingsBase::~CookieSettingsBase() = default;

CookieSettingWithBraveMetadata::CookieSettingWithBraveMetadata() = default;
CookieSettingWithBraveMetadata::CookieSettingWithBraveMetadata(
    const CookieSettingWithBraveMetadata&) = default;
CookieSettingWithBraveMetadata::CookieSettingWithBraveMetadata(
    CookieSettingWithBraveMetadata&&) = default;
CookieSettingWithBraveMetadata& CookieSettingWithBraveMetadata::operator=(
    const CookieSettingWithBraveMetadata&) = default;
CookieSettingWithBraveMetadata& CookieSettingWithBraveMetadata::operator=(
    CookieSettingWithBraveMetadata&&) = default;
CookieSettingWithBraveMetadata::~CookieSettingWithBraveMetadata() = default;

bool CookieSettingWithBraveMetadata::IsExplicitSetting() const {
  return !primary_pattern_matches_all_hosts ||
         !secondary_pattern_matches_all_hosts;
}

bool CookieSettingsBase::ShouldUseEphemeralStorage(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    net::CookieSettingOverrides overrides,
    const std::optional<url::Origin>& top_frame_origin) const {
  if (!base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage))
    return false;

  const GURL first_party_url =
      GetFirstPartyURL(site_for_cookies, base::OptionalToPtr(top_frame_origin));

  if (!first_party_url.is_valid())
    return false;

  // Enable ephemeral storage for a first party URL if SESSION_ONLY cookie
  // setting is set and the feature is enabled.
  std::optional<CookieSettingWithBraveMetadata> first_party_setting;
  if (base::FeatureList::IsEnabled(
          net::features::kBraveFirstPartyEphemeralStorage)) {
    first_party_setting = GetCookieSettingWithBraveMetadata(
        first_party_url, first_party_url, overrides);
    if (IsSessionOnlyExplicit(*first_party_setting)) {
      return true;
    }
  }

  if (net::registry_controlled_domains::SameDomainOrHost(
          first_party_url, url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES))
    return false;

  bool allow_3p =
      IsCookieAccessAllowedImpl(url, site_for_cookies, top_frame_origin,
                                overrides, /*cookie_settings*/ nullptr);
  bool allow_1p = first_party_setting ? IsAllowed(first_party_setting->setting)
                                      : IsFirstPartyAccessAllowed(
                                            first_party_url, this, overrides);

  // only use ephemeral storage for block 3p
  return allow_1p && !allow_3p;
}

bool CookieSettingsBase::IsEphemeralCookieAccessAllowed(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const std::optional<url::Origin>& top_frame_origin,
    net::CookieSettingOverrides overrides) const {
  if (ShouldUseEphemeralStorage(url, site_for_cookies, overrides,
                                top_frame_origin)) {
    return true;
  }

  return IsCookieAccessAllowedImpl(url, site_for_cookies, top_frame_origin,
                                   overrides, /*cookie_settings*/ nullptr);
}

bool CookieSettingsBase::IsFullCookieAccessAllowed(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const std::optional<url::Origin>& top_frame_origin,
    net::CookieSettingOverrides overrides,
    CookieSettingWithMetadata* cookie_settings) const {
  return IsCookieAccessAllowedImpl(url, site_for_cookies, top_frame_origin,
                                   overrides, cookie_settings);
}

bool CookieSettingsBase::IsCookieAccessAllowedImpl(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const std::optional<url::Origin>& top_frame_origin,
    net::CookieSettingOverrides overrides,
    CookieSettingWithMetadata* cookie_settings) const {
  bool allow = IsChromiumFullCookieAccessAllowed(
      url, site_for_cookies, top_frame_origin, overrides, cookie_settings);

  const bool is_1p_ephemeral_feature_enabled = base::FeatureList::IsEnabled(
      net::features::kBraveFirstPartyEphemeralStorage);
  // If 1PES feature is enabled, we should do additional checks below.
  if (allow && !is_1p_ephemeral_feature_enabled)
    return true;

  const GURL first_party_url =
      GetFirstPartyURL(site_for_cookies, base::OptionalToPtr(top_frame_origin));

  // Determine whether a main frame is ephemeral or Shields are down.
  // This is required to properly handle main and nested frames depending on the
  // main frame mode.
  enum class MainFrameMode {
    // Main frame works as usual, nested 3p frames use ephemeral storage if
    // necessary.
    kDefault,
    // Main frame is in Ephemeral Storage mode, 1p/3p frames use ephemeral
    // storage if necessary.
    kEphemeral,
    // Main frame is in "Shields down" mode, all 1p/3p frames should use
    // persistend storage, *including* 3p frames with enabled "First party
    // ephemeral storage" mode.
    kShieldsDown,
  };
  MainFrameMode main_frame_mode = MainFrameMode::kDefault;
  if (is_1p_ephemeral_feature_enabled) {
    // Get CookieSetting for the main frame and get matched patterns if any.
    CookieSettingWithBraveMetadata setting_with_brave_metadata =
        GetCookieSettingWithBraveMetadata(first_party_url, first_party_url,
                                          overrides);

    // Ephemeral mode for the main frame can be enabled only via explicit rule.
    if (IsSessionOnlyExplicit(setting_with_brave_metadata)) {
      main_frame_mode = MainFrameMode::kEphemeral;
    } else {
      // Disabled shields mode allows everything in nested frames. To properly
      // handle this state we need to know if Shields are down in the main
      // frame. The shields check is done by analyzing the primary and secondary
      // patterns and expecting them to be in a specific state.
      if (setting_with_brave_metadata.setting == CONTENT_SETTING_ALLOW &&
          setting_with_brave_metadata.primary_pattern_matches_all_hosts &&
          !setting_with_brave_metadata.secondary_pattern_matches_all_hosts) {
        main_frame_mode = MainFrameMode::kShieldsDown;
      }
    }
  }

  if (allow) {
    // When the main frame is in ephemeral mode, we should block all non
    // ephemeral-supported activities (service workers, etc.).
    if (main_frame_mode == MainFrameMode::kEphemeral) {
      return false;
    }
    return true;
  }

  if (!IsFirstPartyAccessAllowed(first_party_url, this, overrides)) {
    return false;
  }

  if (BraveIsAllowedThirdParty(url, first_party_url, this))
    return true;

  // This allows Session-only frames to work as usual when Shields are down for
  // the main frame.
  if (is_1p_ephemeral_feature_enabled &&
      main_frame_mode == MainFrameMode::kShieldsDown &&
      IsCookieSessionOnly(url)) {
    return true;
  }

  return false;
}

// Determines whether a 3p cookies block should be applied if a requesting URL
// uses an explicit CONTENT_SETTING_SESSION_ONLY setting.
// By default Chromimum allows all 3p cookies if applied CookieSettingsPatterns
// for the URL are explicit, i.e. if a user added the rule manually then they
// trusts the website. We use explicit setting to enable 1PES mode, but in this
// mode we still want to block 3p frames as usual and not fallback to "allow
// everything" path.
bool CookieSettingsBase::ShouldBlockThirdPartyIfSettingIsExplicit(
    bool block_third_party_cookies,
    ContentSetting cookie_setting,
    bool is_explicit_setting,
    bool is_first_party_allowed_scheme) const {
  return block_third_party_cookies &&
         cookie_setting == CONTENT_SETTING_SESSION_ONLY &&
         is_explicit_setting && !is_first_party_allowed_scheme;
}

CookieSettingWithBraveMetadata
CookieSettingsBase::GetCookieSettingWithBraveMetadata(
    const GURL& url,
    const GURL& first_party_url,
    net::CookieSettingOverrides overrides) const {
  CookieSettingWithBraveMetadata setting_brave_metadata;
  const base::AutoReset<CookieSettingWithBraveMetadata*> resetter(
      &current_cookie_settings_with_brave_metadata, &setting_brave_metadata);
  // GetCookieSetting fills metadata structure implicitly (implemented in
  // GetCookieSettingInternal), the setting value is set explicitly here.
  setting_brave_metadata.setting =
      GetCookieSetting(url, first_party_url, overrides, nullptr);
  return setting_brave_metadata;
}

CookieSettingWithBraveMetadata*
CookieSettingsBase::GetCurrentCookieSettingWithBraveMetadata() {
  // Workaround false-positive MSAN use-of-uninitialized-value on
  // thread_local storage for loaded libraries:
  // https://github.com/google/sanitizers/issues/1265
  MSAN_UNPOISON(&current_cookie_settings_with_brave_metadata,
                sizeof(CookieSettingWithBraveMetadata*));

  return current_cookie_settings_with_brave_metadata;
}

}  // namespace content_settings

// Determines whether a 3p cookies block should be applied if a requesting URL
// uses an explicit 1PES setting (CONTENT_SETTING_SESSION_ONLY). By default
// Chromimum allows all 3p cookies if applied CookieSettingsPatterns for the URL
// were explicit. We use explicit setting to enable 1PES mode, but in this mode
// we still want to block 3p frames as usual and not fallback to "allow
// everything" path.
#define BRAVE_COOKIE_SETTINGS_BASE_DECIDE_ACCESS                              \
  const bool block_third =                                                    \
      IsAllowed(setting) && !is_explicit_setting && is_third_party_request && \
      ShouldBlockThirdPartyCookies() &&                                       \
      !IsThirdPartyCookiesAllowedScheme(first_party_url.scheme());            \
  if (!block_third && is_third_party_request &&                               \
      ShouldBlockThirdPartyIfSettingIsExplicit(                               \
          ShouldBlockThirdPartyCookies(), setting, is_explicit_setting,       \
          IsThirdPartyCookiesAllowedScheme(first_party_url.scheme()))) {      \
    return AllowPartitionedCookies{};                                         \
  }

#define BRAVE_COOKIE_SETTINGS_BASE_GET_COOKIES_SETTINGS_INTERNAL         \
  /* Store patterns information to determine if Shields are disabled. */ \
  if (auto* setting_with_brave_metadata =                                \
          GetCurrentCookieSettingWithBraveMetadata()) {                  \
    setting_with_brave_metadata->primary_pattern_matches_all_hosts =     \
        setting_info.primary_pattern.MatchesAllHosts();                  \
    setting_with_brave_metadata->secondary_pattern_matches_all_hosts =   \
        setting_info.secondary_pattern.MatchesAllHosts();                \
  }

// Force GetCookieSetting to use a first-party context when calling into
// GetCookieSettingInternal, rather than assuming a third-party context. See
// https://chromium.googlesource.com/chromium/src/+/1eca8080b750b1b3e3067cfb7209163b9026de8a
#define SiteForCookies() SiteForCookies::FromUrl(first_party_url)

#define IsFullCookieAccessAllowed IsChromiumFullCookieAccessAllowed
#include "src/components/content_settings/core/common/cookie_settings_base.cc"
#undef IsFullCookieAccessAllowed
#undef SiteForCookies
#undef BRAVE_COOKIE_SETTINGS_BASE_GET_COOKIES_SETTINGS_INTERNAL
#undef BRAVE_COOKIE_SETTINGS_BASE_DECIDE_ACCESS
