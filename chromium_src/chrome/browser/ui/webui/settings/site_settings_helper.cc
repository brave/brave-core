/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/site_settings_helper.h"

#include <string_view>
#include <vector>

#include "brave/components/brave_shields/core/common/brave_shield_constants.h"

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
  {ContentSettingsType::BRAVE_WEBCOMPAT_ALL, nullptr},
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

#define kNumSources     \
  kRemoteList:          \
  return "remote-list"; \
  case SiteSettingSource::kNumSources

#include "src/chrome/browser/ui/webui/settings/site_settings_helper.cc"

#undef kNumSources
#undef BRAVE_PROVIDER_TYPE_TO_SITE_SETTINGS_SOURCE
#undef BRAVE_PROVIDER_TO_DEFAULT_SETTINGS_STRING
#undef BRAVE_CONTENT_SETTINGS_TYPE_GROUP_NAMES_LIST
#undef BRAVE_SITE_SETTINGS_HELPER_CONTENT_SETTINGS_TYPE_FROM_GROUP_NAME
#undef BRAVE_SITE_SETTINGS_HELPER_CONTENT_SETTINGS_TYPE_TO_GROUP_NAME
#undef GetVisiblePermissionCategories
#undef HasRegisteredGroupName

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
  if (type == ContentSettingsType::BRAVE_SHIELDS) {
    return true;
  }
  return HasRegisteredGroupName_ChromiumImpl(type);
}

std::vector<ContentSettingsType> GetVisiblePermissionCategories(
    const std::string& origin,
    Profile* profile) {
  static constexpr ContentSettingsType extra_types[] = {
      ContentSettingsType::AUTOPLAY,
      ContentSettingsType::BRAVE_ETHEREUM,
      ContentSettingsType::BRAVE_SOLANA,
      ContentSettingsType::BRAVE_GOOGLE_SIGN_IN,
      ContentSettingsType::BRAVE_LOCALHOST_ACCESS,
      ContentSettingsType::BRAVE_OPEN_AI_CHAT,
  };

  auto types = GetVisiblePermissionCategories_ChromiumImpl(origin, profile);

  types.insert(std::end(types), std::begin(extra_types), std::end(extra_types));
  return types;
}

}  // namespace site_settings
