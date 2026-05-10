/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/site_settings_helper.h"

#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#endif

#include <chrome/browser/ui/webui/settings/site_settings_helper.cc>

namespace site_settings {

bool HasRegisteredGroupName(ContentSettingsType type) {
  if (type == ContentSettingsType::AUTOPLAY) {
    return true;
  }
  if (type == ContentSettingsType::BRAVE_GOOGLE_SIGN_IN) {
    return true;
  }
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  if (type == ContentSettingsType::BRAVE_ETHEREUM) {
    return true;
  }
  if (type == ContentSettingsType::BRAVE_SOLANA) {
    return true;
  }
  if (type == ContentSettingsType::BRAVE_CARDANO) {
    return true;
  }
#endif
  if (type == ContentSettingsType::BRAVE_SHIELDS) {
    return true;
  }
  return HasRegisteredGroupName_ChromiumImpl(type);
}

std::vector<ContentSettingsType> GetVisiblePermissionCategories(
    const std::string& origin,
    Profile* profile) {
  auto types = GetVisiblePermissionCategories_ChromiumImpl(origin, profile);

  // Add Brave-specific content settings types
  types.push_back(ContentSettingsType::AUTOPLAY);
  types.push_back(ContentSettingsType::BRAVE_GOOGLE_SIGN_IN);
  types.push_back(ContentSettingsType::BRAVE_OPEN_AI_CHAT);

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  // Only add Web3-related content settings if wallet is allowed
  if (brave_wallet::IsAllowedForContext(profile)) {
    types.push_back(ContentSettingsType::BRAVE_ETHEREUM);
    types.push_back(ContentSettingsType::BRAVE_SOLANA);
    types.push_back(ContentSettingsType::BRAVE_CARDANO);
  }
#endif

  return types;
}

base::DictValue GetExceptionForPage(
    ContentSettingsType content_type,
    Profile* profile,
    const ContentSettingsPattern& pattern,
    const ContentSettingsPattern& secondary_pattern,
    const std::string& display_name,
    const ContentSetting& setting,
    const SiteSettingSource source,
    const base::Time& expiration,
    bool incognito,
    bool is_embargoed) {
  auto exception = GetExceptionForPage_ChromiumImpl(
      content_type, profile, pattern, secondary_pattern, display_name, setting,
      source, expiration, incognito, is_embargoed);

  // Update the RawSiteException.braveCookieType declaration in
  // site_settings_prefs_browser_proxy.ts if you want to change or add values.
  constexpr char kBraveCookieType[] = "braveCookieType";
  constexpr char kShieldsDown[] = "shields down";
  constexpr char kShieldsSettings[] = "shields settings";
  constexpr char kGoogleSignIn[] = "google sign-in";

  if (content_type == ContentSettingsType::COOKIES) {
    auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
    auto* provider = static_cast<content_settings::BravePrefProvider*>(
        map->GetPrefProvider());
    switch (provider->GetCookieType(pattern, secondary_pattern, setting,
                                    incognito)) {
      case content_settings::BravePrefProvider::CookieType::kRegularCookie:
        break;
      case content_settings::BravePrefProvider::CookieType::kShieldsDownCookie:
        exception.Set(kBraveCookieType, kShieldsDown);
        break;
      case content_settings::BravePrefProvider::CookieType::
          kCustomShieldsCookie:
        exception.Set(kBraveCookieType, kShieldsSettings);
        break;
      case content_settings::BravePrefProvider::CookieType::kGoogleSignInCookie:
        exception.Set(kBraveCookieType, kGoogleSignIn);
        break;
    }
  }

  return exception;
}

}  // namespace site_settings
