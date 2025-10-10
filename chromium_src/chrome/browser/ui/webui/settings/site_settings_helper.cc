/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/site_settings_helper.h"

#include <string_view>
#include <vector>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"
#include "components/content_settings/core/common/content_settings_pattern.h"

#define HasRegisteredGroupName HasRegisteredGroupName_ChromiumImpl
#define GetVisiblePermissionCategories \
  GetVisiblePermissionCategories_ChromiumImpl

// clang-format off
#define BRAVE_CONTENT_SETTINGS_TYPE_GROUP_NAMES_LIST                  \
  {ContentSettingsType::BRAVE_ADS, nullptr},                          \
  {ContentSettingsType::BRAVE_COSMETIC_FILTERING, nullptr},           \
  {ContentSettingsType::BRAVE_TRACKERS, nullptr},                     \
  {ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES, nullptr},    \
  {ContentSettingsType::BRAVE_FINGERPRINTING_V2, nullptr},            \
  {ContentSettingsType::BRAVE_SHIELDS, brave_shields::kBraveShields}, \
  {ContentSettingsType::BRAVE_REFERRERS, nullptr},                    \
  {ContentSettingsType::BRAVE_COOKIES, nullptr},                      \
  {ContentSettingsType::BRAVE_SPEEDREADER, nullptr},                  \
  {ContentSettingsType::BRAVE_ETHEREUM, "ethereum"},                  \
  {ContentSettingsType::BRAVE_SOLANA, "solana"},                      \
  {ContentSettingsType::BRAVE_GOOGLE_SIGN_IN, "googleSignIn"},        \
  {ContentSettingsType::BRAVE_HTTPS_UPGRADE, nullptr},                \
  {ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE, nullptr},          \
  {ContentSettingsType::BRAVE_LOCALHOST_ACCESS, "localhostAccess"},   \
  {ContentSettingsType::BRAVE_OPEN_AI_CHAT, "braveOpenAIChat"},       \
  {ContentSettingsType::BRAVE_WEBCOMPAT_NONE, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_AUDIO, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_CANVAS, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_DEVICE_MEMORY, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_EVENT_SOURCE_POOL, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_FONT, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_KEYBOARD, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_LANGUAGE, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_MEDIA_DEVICES, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_PLUGINS, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_SCREEN, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_SPEECH_SYNTHESIS, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_USB_DEVICE_SERIAL_NUMBER, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_USER_AGENT, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL2, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_WEB_SOCKETS_POOL, nullptr}, \
  {ContentSettingsType::BRAVE_WEBCOMPAT_ALL, nullptr}, \
  {ContentSettingsType::BRAVE_SHIELDS_METADATA, nullptr}, \
  {ContentSettingsType::BRAVE_CARDANO, "cardano"}, \
  {ContentSettingsType::BRAVE_PSST, nullptr}, \
  {ContentSettingsType::BRAVE_SHRED_SITE_DATA, nullptr},
// clang-format on

#define BRAVE_SITE_SETTINGS_HELPER_CONTENT_SETTINGS_TYPE_FROM_GROUP_NAME \
  if (name == "autoplay")                                                \
    return ContentSettingsType::AUTOPLAY;

#define BRAVE_SITE_SETTINGS_HELPER_CONTENT_SETTINGS_TYPE_TO_GROUP_NAME \
  if (type == ContentSettingsType::AUTOPLAY)                           \
    return "autoplay";

#define BRAVE_PROVIDER_TYPE_TO_SITE_SETTINGS_SOURCE \
  case ProviderType::kRemoteListProvider:           \
    return SiteSettingSource::kRemoteList;

#define BRAVE_PROVIDER_TO_DEFAULT_SETTINGS_STRING \
  case ProviderType::kRemoteListProvider:         \
    return "remote_list";

#define BRAVE_GET_EXCEPTION_FOR_PAGE                                  \
  BraveGetExceptionForPage(content_type, profile, incognito, pattern, \
                           secondary_pattern, setting, exception);

#define kNumSources     \
  kRemoteList:          \
  return "remote-list"; \
  case SiteSettingSource::kNumSources

namespace {
// Forward declaration.
void BraveGetExceptionForPage(ContentSettingsType type,
                              Profile* profile,
                              bool incognito,
                              const ContentSettingsPattern& pattern,
                              const ContentSettingsPattern& secondary_pattern,
                              const ContentSetting& setting,
                              base::Value::Dict& exception);
}  // namespace

#include <chrome/browser/ui/webui/settings/site_settings_helper.cc>

#undef kNumSources
#undef BRAVE_PROVIDER_TYPE_TO_SITE_SETTINGS_SOURCE
#undef BRAVE_PROVIDER_TO_DEFAULT_SETTINGS_STRING
#undef BRAVE_CONTENT_SETTINGS_TYPE_GROUP_NAMES_LIST
#undef BRAVE_SITE_SETTINGS_HELPER_CONTENT_SETTINGS_TYPE_FROM_GROUP_NAME
#undef BRAVE_SITE_SETTINGS_HELPER_CONTENT_SETTINGS_TYPE_TO_GROUP_NAME
#undef GetVisiblePermissionCategories
#undef HasRegisteredGroupName
#undef BRAVE_GET_EXCEPTION_FOR_PAGE

namespace site_settings {

bool HasRegisteredGroupName(ContentSettingsType type) {
  if (type == ContentSettingsType::AUTOPLAY) {
    return true;
  }
  if (type == ContentSettingsType::BRAVE_GOOGLE_SIGN_IN) {
    return true;
  }
  if (type == ContentSettingsType::BRAVE_LOCALHOST_ACCESS) {
    return true;
  }
  if (type == ContentSettingsType::BRAVE_ETHEREUM) {
    return true;
  }
  if (type == ContentSettingsType::BRAVE_SOLANA) {
    return true;
  }
  if (type == ContentSettingsType::BRAVE_CARDANO) {
    return true;
  }
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
  types.push_back(ContentSettingsType::BRAVE_LOCALHOST_ACCESS);
  types.push_back(ContentSettingsType::BRAVE_OPEN_AI_CHAT);

  // Only add Web3-related content settings if wallet is allowed
  if (brave_wallet::IsAllowedForContext(profile)) {
    types.push_back(ContentSettingsType::BRAVE_ETHEREUM);
    types.push_back(ContentSettingsType::BRAVE_SOLANA);
    types.push_back(ContentSettingsType::BRAVE_CARDANO);
  }

  return types;
}
}  // namespace site_settings

namespace {
void BraveGetExceptionForPage(ContentSettingsType type,
                              Profile* profile,
                              bool incognito,
                              const ContentSettingsPattern& pattern,
                              const ContentSettingsPattern& secondary_pattern,
                              const ContentSetting& setting,
                              base::Value::Dict& exception) {
  constexpr char kBraveCookieType[] = "braveCookieType";

  // Update the RawSiteException.braveCookieType declaration in
  // site_settings_prefs_browser_proxy.ts if you want to change or add values.
  constexpr char kShieldsDown[] = "shields down";
  constexpr char kShieldsSettings[] = "shields settings";
  constexpr char kGoogleSignIn[] = "google sign-in";

  if (type == ContentSettingsType::COOKIES) {
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
}

}  // namespace
