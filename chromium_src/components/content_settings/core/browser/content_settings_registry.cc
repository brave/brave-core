/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_INIT { \
  /* Add CONTENT_SETTING_ASK and make it default for autoplay*/ \
  content_settings_info_.erase(ContentSettingsType::AUTOPLAY); \
  website_settings_registry_->UnRegister(ContentSettingsType::AUTOPLAY); \
  Register(ContentSettingsType::AUTOPLAY, "autoplay", CONTENT_SETTING_BLOCK, \
           WebsiteSettingsInfo::UNSYNCABLE, WhitelistedSchemes(), \
           ValidSettings(CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK, \
                         CONTENT_SETTING_ASK), \
           WebsiteSettingsInfo::SINGLE_ORIGIN_ONLY_SCOPE, \
           WebsiteSettingsRegistry::DESKTOP | \
               WebsiteSettingsRegistry::PLATFORM_ANDROID, \
           ContentSettingsInfo::INHERIT_IN_INCOGNITO, \
           ContentSettingsInfo::PERSISTENT, \
           ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS); \
  \
  /* Change plugins default to CONTENT_SETTING_BLOCK*/ \
  content_settings_info_.erase(ContentSettingsType::PLUGINS); \
  website_settings_registry_->UnRegister(ContentSettingsType::PLUGINS); \
  Register( \
      ContentSettingsType::PLUGINS, "plugins", \
      CONTENT_SETTING_BLOCK, WebsiteSettingsInfo::SYNCABLE, \
      WhitelistedSchemes(kChromeUIScheme, kChromeDevToolsScheme), \
      ValidSettings(CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK, \
                    CONTENT_SETTING_ASK, \
                    CONTENT_SETTING_DETECT_IMPORTANT_CONTENT), \
      WebsiteSettingsInfo::SINGLE_ORIGIN_WITH_EMBEDDED_EXCEPTIONS_SCOPE, \
      WebsiteSettingsRegistry::DESKTOP | \
          WebsiteSettingsRegistry::PLATFORM_ANDROID, \
      ContentSettingsInfo::INHERIT_IN_INCOGNITO, \
      ContentSettingsInfo::EPHEMERAL, \
      ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS); \
  \
  /* Disable background sync by default (brave/brave-browser#4709)*/ \
  content_settings_info_.erase(ContentSettingsType::BACKGROUND_SYNC); \
  website_settings_registry_->UnRegister( \
      ContentSettingsType::BACKGROUND_SYNC); \
  Register( \
      ContentSettingsType::BACKGROUND_SYNC, "background-sync", \
      CONTENT_SETTING_BLOCK, WebsiteSettingsInfo::UNSYNCABLE, \
      WhitelistedSchemes(), \
      ValidSettings(CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK), \
      WebsiteSettingsInfo::SINGLE_ORIGIN_ONLY_SCOPE, \
      WebsiteSettingsRegistry::DESKTOP | \
          WebsiteSettingsRegistry::PLATFORM_ANDROID, \
      ContentSettingsInfo::INHERIT_IN_INCOGNITO, \
      ContentSettingsInfo::PERSISTENT, \
      ContentSettingsInfo::EXCEPTIONS_ON_SECURE_ORIGINS_ONLY); \
  \
  /* Disable motion sensors by default (brave/brave-browser#4789)*/ \
  content_settings_info_.erase(ContentSettingsType::SENSORS); \
  website_settings_registry_->UnRegister(ContentSettingsType::SENSORS); \
  Register( \
      ContentSettingsType::SENSORS, "sensors", CONTENT_SETTING_BLOCK, \
      WebsiteSettingsInfo::UNSYNCABLE, WhitelistedSchemes(), \
      ValidSettings(CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK), \
      WebsiteSettingsInfo::SINGLE_ORIGIN_ONLY_SCOPE, \
      WebsiteSettingsRegistry::DESKTOP | \
          WebsiteSettingsRegistry::PLATFORM_ANDROID, \
      ContentSettingsInfo::INHERIT_IN_INCOGNITO, \
      ContentSettingsInfo::PERSISTENT, \
      ContentSettingsInfo::EXCEPTIONS_ON_SECURE_AND_INSECURE_ORIGINS); \
}

#include "../../../../../../components/content_settings/core/browser/content_settings_registry.cc"

#undef BRAVE_INIT
