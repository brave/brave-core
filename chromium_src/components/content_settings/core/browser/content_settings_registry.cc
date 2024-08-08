/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_INIT BraveInit();
#include "src/components/content_settings/core/browser/content_settings_registry.cc"
#undef BRAVE_INIT

#include "base/containers/fixed_flat_map.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings.mojom.h"
#include "net/base/features.h"

namespace content_settings {

namespace {
using enum ContentSettingsType;
constexpr auto kSettingsNames =
    base::MakeFixedFlatMap<ContentSettingsType, const char*>({
        {BRAVE_WEBCOMPAT_NONE, "brave-webcompat-none"},
        {BRAVE_WEBCOMPAT_AUDIO, "brave-webcompat-audio"},
        {BRAVE_WEBCOMPAT_CANVAS, "brave-webcompat-canvas"},
        {BRAVE_WEBCOMPAT_DEVICE_MEMORY, "brave-webcompat-device-memory"},
        {BRAVE_WEBCOMPAT_EVENT_SOURCE_POOL,
         "brave-webcompat-event-source-pool"},
        {BRAVE_WEBCOMPAT_FONT, "brave-webcompat-font"},
        {BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY,
         "brave-webcompat-hardware-concurrency"},
        {BRAVE_WEBCOMPAT_KEYBOARD, "brave-webcompat-keyboard"},
        {BRAVE_WEBCOMPAT_LANGUAGE, "brave-webcompat-language"},
        {BRAVE_WEBCOMPAT_MEDIA_DEVICES, "brave-webcompat-media-devices"},
        {BRAVE_WEBCOMPAT_PLUGINS, "brave-webcompat-plugins"},
        {BRAVE_WEBCOMPAT_SCREEN, "brave-webcompat-screen"},
        {BRAVE_WEBCOMPAT_SPEECH_SYNTHESIS, "brave-webcompat-speech-synthesis"},
        {BRAVE_WEBCOMPAT_USB_DEVICE_SERIAL_NUMBER,
         "brave-webcompat-usb-device-serial-number"},
        {BRAVE_WEBCOMPAT_USER_AGENT, "brave-webcompat-user-agent"},
        {BRAVE_WEBCOMPAT_WEBGL, "brave-webcompat-webgl"},
        {BRAVE_WEBCOMPAT_WEBGL2, "brave-webcompat-webgl2"},
        {BRAVE_WEBCOMPAT_WEB_SOCKETS_POOL, "brave-webcompat-web-sockets-pool"},
    });
}  // namespace

void ContentSettingsRegistry::BraveInit() {
  Register(ContentSettingsType::BRAVE_ADS, brave_shields::kAds,
           CONTENT_SETTING_BLOCK, WebsiteSettingsInfo::SYNCABLE,
           /*allowlisted_schemes=*/{},
           /*valid_settings=*/{CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  Register(ContentSettingsType::BRAVE_TRACKERS, brave_shields::kTrackers,
           CONTENT_SETTING_BLOCK, WebsiteSettingsInfo::SYNCABLE,
           /*allowlisted_schemes=*/{},
           /*valid_settings=*/{CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  Register(ContentSettingsType::BRAVE_HTTPS_UPGRADE,
           brave_shields::kHTTPSUpgrades, CONTENT_SETTING_ASK,
           WebsiteSettingsInfo::SYNCABLE, /*allowlisted_schemes=*/{},
           /*valid_settings=*/
           {CONTENT_SETTING_ALLOW, CONTENT_SETTING_ASK, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IF_LESS_PERMISSIVE,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  Register(ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
           brave_shields::kHTTPUpgradableResources, CONTENT_SETTING_BLOCK,
           WebsiteSettingsInfo::SYNCABLE, /*allowlisted_schemes=*/{},
           /*valid_settings=*/{CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  Register(ContentSettingsType::BRAVE_REFERRERS, brave_shields::kReferrers,
           CONTENT_SETTING_BLOCK, WebsiteSettingsInfo::SYNCABLE,
           /*allowlisted_schemes=*/{},
           /*valid_settings=*/{CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  Register(
      ContentSettingsType::BRAVE_COOKIES, brave_shields::kCookies,
      CONTENT_SETTING_DEFAULT, WebsiteSettingsInfo::SYNCABLE,
      /*allowlisted_schemes=*/{kChromeUIScheme, kChromeDevToolsScheme},
      /*valid_settings=*/{CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK},
      WebsiteSettingsInfo::REQUESTING_ORIGIN_WITH_TOP_ORIGIN_EXCEPTIONS_SCOPE,
      WebsiteSettingsRegistry::DESKTOP |
          WebsiteSettingsRegistry::PLATFORM_ANDROID,
      ContentSettingsInfo::INHERIT_IN_INCOGNITO,
      ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  Register(
      ContentSettingsType::BRAVE_COSMETIC_FILTERING,
      brave_shields::kCosmeticFiltering, CONTENT_SETTING_DEFAULT,
      WebsiteSettingsInfo::SYNCABLE, /*allowlisted_schemes=*/{},
      /*valid_settings=*/{CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK},
      WebsiteSettingsInfo::REQUESTING_ORIGIN_WITH_TOP_ORIGIN_EXCEPTIONS_SCOPE,
      WebsiteSettingsRegistry::DESKTOP |
          WebsiteSettingsRegistry::PLATFORM_ANDROID,
      ContentSettingsInfo::INHERIT_IN_INCOGNITO,
      ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  Register(ContentSettingsType::BRAVE_FINGERPRINTING_V2,
           brave_shields::kFingerprintingV2, CONTENT_SETTING_ASK,
           WebsiteSettingsInfo::SYNCABLE, /*allowlisted_schemes=*/{},
           /*valid_settings=*/
           {CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK, CONTENT_SETTING_ASK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  Register(ContentSettingsType::BRAVE_SHIELDS, brave_shields::kBraveShields,
           CONTENT_SETTING_ALLOW, WebsiteSettingsInfo::SYNCABLE,
           /*allowlisted_schemes=*/{},
           /*valid_settings=*/{CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  Register(ContentSettingsType::BRAVE_SPEEDREADER, "braveSpeedreader",
           CONTENT_SETTING_DEFAULT, WebsiteSettingsInfo::SYNCABLE,
           /*allowlisted_schemes=*/{},
           /*valid_settings=*/{CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  // Add CONTENT_SETTING_ASK for autoplay
  // Note ASK has been deprecated, only keeping it for
  // DiscardObsoleteAutoplayAsk test case
  content_settings_info_.erase(ContentSettingsType::AUTOPLAY);
  website_settings_registry_->UnRegister(ContentSettingsType::AUTOPLAY);
  Register(ContentSettingsType::AUTOPLAY, "autoplay", CONTENT_SETTING_ALLOW,
           WebsiteSettingsInfo::UNSYNCABLE, /*allowlisted_schemes=*/{},
           /*valid_settings=*/
           {CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK, CONTENT_SETTING_ASK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  // Register ethereum default value as Ask.
  Register(ContentSettingsType::BRAVE_ETHEREUM, "brave_ethereum",
           CONTENT_SETTING_ASK, WebsiteSettingsInfo::UNSYNCABLE,
           /*allowlisted_schemes=*/{},
           /*valid_settings=*/
           {CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK, CONTENT_SETTING_ASK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IF_LESS_PERMISSIVE,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  // Register solana default value as Ask.
  Register(ContentSettingsType::BRAVE_SOLANA, "brave_solana",
           CONTENT_SETTING_ASK, WebsiteSettingsInfo::UNSYNCABLE,
           /*allowlisted_schemes=*/{},
           /*valid_settings=*/
           {CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK, CONTENT_SETTING_ASK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IF_LESS_PERMISSIVE,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  // Register google sign in social media permission default value as Ask.
  // This is INHERIT_IN_INCOGNITO because it sets cookie rules, and cookies
  // are INHERIT_IN_INCOGNITO.
  // See https://github.com/brave/brave-core/pull/15330#discussion_r1049643580
  Register(ContentSettingsType::BRAVE_GOOGLE_SIGN_IN, "brave_google_sign_in",
           CONTENT_SETTING_ASK, WebsiteSettingsInfo::UNSYNCABLE,
           /*allowlisted_schemes=*/{},
           /*valid_settings=*/
           {CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK, CONTENT_SETTING_ASK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  Register(ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
           "brave_remember_1p_storage",
           net::features::kBraveForgetFirstPartyStorageByDefault.Get()
               ? CONTENT_SETTING_BLOCK
               : CONTENT_SETTING_ALLOW,
           WebsiteSettingsInfo::UNSYNCABLE, {},
           {CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  // Register localhost permission default value as Ask.
  Register(ContentSettingsType::BRAVE_LOCALHOST_ACCESS,
           "brave_localhost_access", CONTENT_SETTING_ASK,
           WebsiteSettingsInfo::UNSYNCABLE,
           /*allowlisted_schemes=*/{},
           /*valid_settings=*/
           {CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK, CONTENT_SETTING_ASK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IF_LESS_PERMISSIVE,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  // Disable background sync by default (brave/brave-browser#4709)
  content_settings_info_.erase(ContentSettingsType::BACKGROUND_SYNC);
  website_settings_registry_->UnRegister(ContentSettingsType::BACKGROUND_SYNC);
  Register(ContentSettingsType::BACKGROUND_SYNC, "background-sync",
           CONTENT_SETTING_BLOCK, WebsiteSettingsInfo::UNSYNCABLE,
           /*allowlisted_schemes=*/{},
           /*valid_settings=*/{CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_ORIGINS_ONLY);

  // Disable motion sensors by default (brave/brave-browser#4789)
  content_settings_info_.erase(ContentSettingsType::SENSORS);
  website_settings_registry_->UnRegister(ContentSettingsType::SENSORS);
  Register(ContentSettingsType::SENSORS, "sensors", CONTENT_SETTING_BLOCK,
           WebsiteSettingsInfo::UNSYNCABLE, /*allowlisted_schemes=*/{},
           /*valid_settings=*/{CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  // Disable idle detection by default (we used to disable feature flag
  // kIdleDetection, but it went away in cr121).
  content_settings_info_.erase(ContentSettingsType::IDLE_DETECTION);
  website_settings_registry_->UnRegister(ContentSettingsType::IDLE_DETECTION);
  Register(ContentSettingsType::IDLE_DETECTION, "idle-detection",
           CONTENT_SETTING_BLOCK, WebsiteSettingsInfo::UNSYNCABLE,
           /*allowlisted_primary_schemes=*/{},
           /*valid_settings=*/
           {CONTENT_SETTING_ALLOW, CONTENT_SETTING_ASK, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::ALL_PLATFORMS,
           ContentSettingsInfo::INHERIT_IF_LESS_PERMISSIVE,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_ORIGINS_ONLY);

  // Disable storage access by default (we used to disable feature flag
  // kPermissionStorageAccessAPI, but it went away in cr124).
  content_settings_info_.erase(ContentSettingsType::STORAGE_ACCESS);
  website_settings_registry_->UnRegister(ContentSettingsType::STORAGE_ACCESS);
  content_settings_info_.erase(ContentSettingsType::TOP_LEVEL_STORAGE_ACCESS);
  website_settings_registry_->UnRegister(
      ContentSettingsType::TOP_LEVEL_STORAGE_ACCESS);
  Register(ContentSettingsType::STORAGE_ACCESS, "storage-access",
           CONTENT_SETTING_BLOCK, WebsiteSettingsInfo::UNSYNCABLE,
           /*allowlisted_primary_schemes=*/{},
           /*valid_settings=*/
           {CONTENT_SETTING_ALLOW, CONTENT_SETTING_ASK, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::REQUESTING_AND_TOP_SCHEMEFUL_SITE_SCOPE,
           WebsiteSettingsRegistry::ALL_PLATFORMS,
           ContentSettingsInfo::INHERIT_IF_LESS_PERMISSIVE,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);
  Register(ContentSettingsType::TOP_LEVEL_STORAGE_ACCESS,
           "top-level-storage-access", CONTENT_SETTING_BLOCK,
           WebsiteSettingsInfo::UNSYNCABLE,
           /*allowlisted_primary_schemes=*/{},
           /*valid_settings=*/
           {CONTENT_SETTING_ALLOW, CONTENT_SETTING_ASK, CONTENT_SETTING_BLOCK},
           WebsiteSettingsInfo::REQUESTING_ORIGIN_AND_TOP_SCHEMEFUL_SITE_SCOPE,
           WebsiteSettingsRegistry::ALL_PLATFORMS,
           ContentSettingsInfo::INHERIT_IF_LESS_PERMISSIVE,
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);

  website_settings_registry_->UnRegister(ContentSettingsType::HTTP_ALLOWED);
  website_settings_registry_->Register(
      ContentSettingsType::HTTP_ALLOWED, "http-allowed", base::Value(),
      WebsiteSettingsInfo::UNSYNCABLE, WebsiteSettingsInfo::NOT_LOSSY,
      WebsiteSettingsInfo::GENERIC_SINGLE_ORIGIN_SCOPE,
      WebsiteSettingsRegistry::DESKTOP |
          WebsiteSettingsRegistry::PLATFORM_ANDROID,
      WebsiteSettingsInfo::DONT_INHERIT_IN_INCOGNITO);

  for (auto settings_type = ContentSettingsType::BRAVE_WEBCOMPAT_NONE;
       settings_type != ContentSettingsType::BRAVE_WEBCOMPAT_ALL;
       settings_type = static_cast<ContentSettingsType>(
           static_cast<int32_t>(settings_type) + 1)) {
    const auto match = kSettingsNames.find(settings_type);
    if (match != kSettingsNames.end()) {
      Register(
          settings_type, match->second, CONTENT_SETTING_ASK,
          WebsiteSettingsInfo::UNSYNCABLE, /*allowlisted_schemes=*/{},
          /*valid_settings=*/
          {CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK, CONTENT_SETTING_ASK},
          WebsiteSettingsInfo::TOP_ORIGIN_ONLY_SCOPE,
          WebsiteSettingsRegistry::DESKTOP |
              WebsiteSettingsRegistry::PLATFORM_ANDROID,
          ContentSettingsInfo::INHERIT_IN_INCOGNITO,
          ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS);
    }
  }
}

}  // namespace content_settings
